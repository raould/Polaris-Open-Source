// POLARIS SYSTEM MONITOR

#include "ntddk.h"

#include "poladvr.h"


// Begin: kernel memory allocation helpers.
#define SPACE_BANK 'alop'

PVOID Alloc(SIZE_T bytes) {
    return ExAllocatePoolWithTag(NonPagedPool, bytes, SPACE_BANK);
}

VOID Free(PVOID p) { ExFreePoolWithTag(p, SPACE_BANK); }
// End: kernel memory allocation helpers.

// Begin: driver state.
typedef struct _ProcessLock {
    struct _ProcessLock* bucket;
    struct _ProcessLock* next;
    struct _ProcessLock* previous;

    HANDLE id;
    KEVENT ready;
} ProcessLock;

ProcessLock* AllocProcessLock(HANDLE id) {
    ProcessLock* r = (ProcessLock*)Alloc(sizeof(ProcessLock));
    r->id = id;
    return r;
}

KMUTEX m_mutex;
PKEVENT m_waiting_signal = NULL;
LUID m_non_pet_privilege;
size_t m_waiting_count = 0;   // The number of waiting processes.
ProcessLock* m_sentinel;
ProcessLock* m_pending[7];
const m_pending_size = sizeof m_pending / sizeof(ProcessLock*);

NTSTATUS Enter() {
    return KeWaitForMutexObject(&m_mutex, Executive, KernelMode, FALSE, NULL);
}

VOID Leave() { KeReleaseMutex(&m_mutex, FALSE); }

SIZE_T Hash(HANDLE id) { return (((SIZE_T)id) >> 1) % m_pending_size; }

VOID Construct() {
    SIZE_T i;

    KeInitializeMutex(&m_mutex, 0);
    m_waiting_signal = NULL;
    m_waiting_count = 0;
    for (i = m_pending_size; i-- != 0;) {
        m_pending[i] = NULL;
    }
    m_sentinel = AllocProcessLock(0);
    m_sentinel->bucket = NULL;
    m_sentinel->next = m_sentinel;
    m_sentinel->previous = m_sentinel;
}

VOID Destruct() {
    ProcessLock* i;

    Enter();

    if (m_sentinel->next != m_sentinel) {
        DbgPrint("poladvr: orphan processes ( ");
        for (i = m_sentinel->next; i != m_sentinel; i = i->next) {
            DbgPrint(" %d", i->id);
        }
        DbgPrint(" )\n");
    }

    Free(m_sentinel);
    m_sentinel = NULL;

    Leave();
}
// End: driver state.

// Begin: driver state interface.
NTSTATUS Enable(HANDLE waiting_signal, LUID non_pet_privilege) {
    NTSTATUS error;

    Enter();

    // Get the reference to the powerbox.
    error = ObReferenceObjectByHandle(waiting_signal,
                                      EVENT_MODIFY_STATE,
                                      *ExEventObjectType,
                                      UserMode,
                                      (PVOID*)&m_waiting_signal,
                                      NULL);
    if (NT_SUCCESS(error)) {
        m_non_pet_privilege = non_pet_privilege;
        DbgPrint("poladvr: exception for non-pet privilege LUID ( %d, %d )\n",
                 m_non_pet_privilege.LowPart, m_non_pet_privilege.HighPart);
        DbgPrint("poladvr: enabled");
    } else {
        switch (error) {
        case STATUS_OBJECT_TYPE_MISMATCH:
            DbgPrint("poladvr: not an event object handle");
            break;
        case STATUS_ACCESS_DENIED:
            DbgPrint("poladvr: access denied to event");
            break;
        case STATUS_INVALID_HANDLE:
            DbgPrint("poladvr: invalid event object handle");
            break;
        default:
            DbgPrint("poladvr: event object error: %d", error);
        }
    }

    Leave();

    return error;
}

VOID Disable() {
    SIZE_T i;
    ProcessLock* p;
    ProcessLock* tmp;

    Enter();

    // Free all blocked processes.
    for (i = m_pending_size; i-- != 0;) {
        p = m_pending[i];
        while (p) {
            // Release any waiting thread.
            if (!KeSetEvent(&p->ready, 0, FALSE)) {
                DbgPrint("poladvr: released %d\n", p->id);
                --m_waiting_count;
                if (0 == m_waiting_count) {
                    if (m_waiting_signal) {
                        KeClearEvent(m_waiting_signal);
                    }
                    DbgPrint("poladvr: no processes waiting");
                }
            }

            // Free the state.
            tmp = p->bucket;
            Free(p);
            p = tmp;
        }
        m_pending[i] = NULL;
    }

    // Release the reference to the powerbox.
    if (m_waiting_signal) {
        ObDereferenceObject(m_waiting_signal);
        m_waiting_signal = NULL;
        DbgPrint("poladvr: disabled");
    }

    Leave();
}

HANDLE PeekProcess() {
    HANDLE r;
    ProcessLock* i;

    Enter();
    for (i = m_sentinel->next; i != m_sentinel; i = i->next) {
        if (!KeReadStateEvent(&i->ready)) {
            break;
        }
    }
    r = i->id;
    Leave();

    return r;
}

VOID CheckProcess(const HANDLE id) {
    ProcessLock** parent;
    ProcessLock* i;

    Enter();
    if (m_waiting_signal) {
        parent = m_pending + Hash(id);
        i = *parent;
        while (i) {
            if (id == i->id) {
                break;
            }
            parent = &i->bucket;
            i = *parent;
        }
        if (!i) {
            i = AllocProcessLock(id);
            KeInitializeEvent(&i->ready, NotificationEvent, TRUE);
            i->bucket = NULL;
            *parent = i;
            i->next = m_sentinel;
            i->previous = m_sentinel->previous;
            m_sentinel->previous = i;
            i->previous->next = i;
            DbgPrint("poladvr: checking %d\n", id);
        }
    }
    Leave();
}

VOID ReleaseProcess(const HANDLE id) {
    ProcessLock** parent;
    ProcessLock* i;

    Enter();
    parent = m_pending + Hash(id);
    i = *parent;
    while (i) {
        if (id == i->id) {
            // Release any waiting thread.
            if (!KeSetEvent(&i->ready, 0, FALSE)) {
                DbgPrint("poladvr: released %d\n", id);
                --m_waiting_count;
                if (0 == m_waiting_count) {
                    KeClearEvent(m_waiting_signal);
                    DbgPrint("poladvr: no processes waiting");
                }
            }

            // Free the state.
            *parent = i->bucket;
            i->previous->next = i->next;
            i->next->previous = i->previous;
            Free(i);
            break;
        }
        parent = &i->bucket;
        i = *parent;
    }
    Leave();
}

VOID ContinueProcess(const HANDLE id) {
    ProcessLock* i;

    Enter();
    for (i = m_pending[Hash(id)]; i; i = i->bucket) {
        if (id == i->id) {
            if (!KeSetEvent(&i->ready, 0, FALSE)) {
                DbgPrint("poladvr: continued %d\n", id);
                --m_waiting_count;
                if (0 == m_waiting_count) {
                    KeClearEvent(m_waiting_signal);
                    DbgPrint("poladvr: no processes waiting");
                }
            }
            break;
        }
    }
    Leave();
}

VOID GuardProcess(const HANDLE id) {
    NTSTATUS error;
    ProcessLock** parent;
    ProcessLock* i;
    KEVENT* lock = NULL;

    Enter();
    parent = m_pending + Hash(id);
    i = *parent;
    while (i) {
        if (id == i->id) {
            if (SeSinglePrivilegeCheck(m_non_pet_privilege, UserMode)) {
                // This must be a non-pet process.
                DbgPrint("poladvr: released non-pet %d\n", id);

                // Free the state.
                *parent = i->bucket;
                i->previous->next = i->next;
                i->next->previous = i->previous;
                Free(i);
            } else {
                // This might be a pet process.
                lock = &i->ready;
                if (KeResetEvent(lock)) {
                    if (0 == m_waiting_count) {
                        KeSetEvent(m_waiting_signal, 0, FALSE);
                    }
                    ++m_waiting_count;
                }
            }
            break;
        }
        parent = &i->bucket;
        i = *parent;
    }
    Leave();
    if (lock) {
        DbgPrint("poladvr: %d waiting to be checked...\n", id);
        error = KeWaitForSingleObject(lock, UserRequest, KernelMode,
                                      FALSE, NULL);
        if (NT_SUCCESS(error)) {
            DbgPrint("poladvr: %d is resuming!\n", id);
        } else {
            switch (error) {
            case STATUS_TIMEOUT:
                DbgPrint("poladvr: %d timed out!\n", id);
                break;
            default:
                DbgPrint("poladvr: %d wait error: %d!\n", id, error);
            }
        }
    }
}
// Begin: driver state interface.

// Begin: process and thread monitoring
VOID ProcessCallback(IN HANDLE parent, IN HANDLE process, IN BOOLEAN created) {
    if (created) {
        CheckProcess(process);
    } else {
        ReleaseProcess(process);
    }
}

VOID LoadCallback(IN PUNICODE_STRING image,
                  IN HANDLE process, IN PIMAGE_INFO info) {
    // DbgPrint("poladvr: loading %ws in %d\n", image->Buffer, process);
    GuardProcess(process);
}

NTSTATUS OnIRP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS OnClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    Disable();
    return OnIRP(DeviceObject, Irp);
}

NTSTATUS OnCommand(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS error;
    size_t i;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOID input = Irp->AssociatedIrp.SystemBuffer;
    ULONG input_length = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    PVOID output = Irp->AssociatedIrp.SystemBuffer;
    ULONG output_length = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG FunctionCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    switch (FunctionCode) {
    case IOCTL_POLADVR_SETUP:
        if (input && sizeof(PoladvrSetupArgs) <= input_length) {
            const PoladvrSetupArgs* args = (PoladvrSetupArgs*)input;
            error = Enable(args->waiting_signal, args->non_pet_privilege);
        } else {
            DbgPrint("poladvr: bad setup arguments: %d", input_length);
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        }
        break;
    case IOCTL_POLADVR_CHECK:
        if (sizeof(HANDLE) <= output_length && output) {
            *(HANDLE*)output = PeekProcess();
            Irp->IoStatus.Information = sizeof(HANDLE);
        } else {
            DbgPrint("poladvr: bad check arguments: %d", output_length);
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        }
        break;
    case IOCTL_POLADVR_RELEASE:
        if (sizeof(HANDLE) <= input_length && input) {
            ReleaseProcess(*(HANDLE*)input);
        } else {
            DbgPrint("poladvr: bad release arguments: %d", input_length);
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        }
        break;
    case IOCTL_POLADVR_CONTINUE:
        if (sizeof(HANDLE) <= input_length && input) {
            ContinueProcess(*(HANDLE*)input);
        } else {
            DbgPrint("poladvr: bad continue arguments: %d", input_length);
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        }
        break;
    default:
        DbgPrint("poladvr: unknown command: %d", FunctionCode);
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Irp->IoStatus.Status;
}

/**
 * Remove the process and thread notify routines.
 */
VOID OnUnload( IN PDRIVER_OBJECT DriverObject ) {
    NTSTATUS error;
    const WCHAR device_link[] = L"\\DosDevices\\POLADVR";
    UNICODE_STRING device_link_unicode;

    DbgPrint("poladvr: unloading...\n");

    // Delete the symbolic link.
    RtlInitUnicodeString(&device_link_unicode, device_link);
    IoDeleteSymbolicLink(&device_link_unicode);

    // Delete the device object.
    if (DriverObject->DeviceObject) {
        IoDeleteDevice(DriverObject->DeviceObject);
    }

    // Remove the load image observer.
    error = PsRemoveLoadImageNotifyRoutine(LoadCallback);
    if (NT_SUCCESS(error)) {
        DbgPrint("poladvr: ignoring image load\n");
    } else {
        switch (error) {
        case STATUS_PROCEDURE_NOT_FOUND:
            DbgPrint("poladvr: load monitor was not registered\n");
            break;
        default:
            DbgPrint("poladvr: load monitor removal failed: %d\n", error);
        }
    }

    // Remove the process observer.
    error = PsSetCreateProcessNotifyRoutine(ProcessCallback, TRUE);
    if (NT_SUCCESS(error)) {
        DbgPrint("poladvr: ignoring process creation\n");
    } else {
        switch (error) {
        case STATUS_INVALID_PARAMETER:
            DbgPrint("poladvr: process monitor was not registered\n");
            break;
        default:
            DbgPrint("poladvr: process monitor removal failed: %d\n", error);
        }
    }

    // Clean up state.
    Destruct();

    DbgPrint("poladvr: unloading done");
}

/**
 * Add the process and thread notify routines.
 */
NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject,
                      IN PUNICODE_STRING theRegistryPath ) {
    NTSTATUS error;
    int i;
    PDEVICE_OBJECT device = NULL;
    const WCHAR device_name[] = L"\\Device\\POLADVR";
    UNICODE_STRING device_name_unicode;
    const WCHAR device_link[] = L"\\DosDevices\\POLADVR";
    UNICODE_STRING device_link_unicode;

    DbgPrint("poladvr: loading...");

    // Register all the event handlers.
    Construct();
    theDriverObject->DriverUnload  = OnUnload;
    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i) {
        theDriverObject->MajorFunction[i] = OnIRP;
    }
    theDriverObject->MajorFunction[IRP_MJ_CLOSE] = OnClose;
    theDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = OnCommand;

    // Add the process observer.
    error = PsSetCreateProcessNotifyRoutine(ProcessCallback, FALSE);
    if (NT_SUCCESS(error)) {
        DbgPrint("poladvr: watching process creation...\n");

        // Add the load observer.
        error = PsSetLoadImageNotifyRoutine(LoadCallback);
        if (NT_SUCCESS(error)) {
            DbgPrint("poladvr: watching image load...\n");
        } else {
            switch (error) {
            case STATUS_INSUFFICIENT_RESOURCES:
                DbgPrint("poladvr: cannot add load monitor\n");
                break;
            default:
                DbgPrint("poladvr: monitor load failed: %d\n", error);
            }
        }
    } else {
        switch (error) {
        case STATUS_INVALID_PARAMETER:
            DbgPrint("poladvr: cannot add process monitor\n");
            break;
        default:
            DbgPrint("poladvr: monitor process failed: %d\n", error);
        }
    }

    // Set up a named device.
    RtlInitUnicodeString(&device_name_unicode, device_name);
    error = IoCreateDevice(theDriverObject,
                           0,
                           &device_name_unicode,
                           0x00001234,
                           0,
                           TRUE,
                           &device);
    if (NT_SUCCESS(error)) {

        // Create a symbolic link that the powerbox will use to connect.
        RtlInitUnicodeString(&device_link_unicode, device_link);
        error = IoCreateSymbolicLink(&device_link_unicode,
                                     &device_name_unicode);
        if (NT_SUCCESS(error)) {
            DbgPrint("poladvr: ready for setup\n");
        } else {
            DbgPrint("poladvr: device link failed: %d\n", error);
        }
    } else {
        switch (error) {
        case STATUS_OBJECT_NAME_EXISTS:
            DbgPrint("poladvr: device name exists\n");
            break;
        case STATUS_OBJECT_NAME_COLLISION:
            DbgPrint("poladvr: device name collision\n");
            break;
        default:
            DbgPrint("poladvr: device name failed: %d\n", error);
        }
    }

    return STATUS_SUCCESS;
}

