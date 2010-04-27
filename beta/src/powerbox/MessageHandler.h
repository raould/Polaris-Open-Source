#pragma once

struct MessageResponder;

struct MessageHandler
{
	virtual ~MessageHandler() {}

	/**
	 * Do the action.
	 * @param PID	The requesting process identifier.
	 * @param args	The list of input strings.
	 * @param out	The request responder.
	 */
	virtual void run(int PID, const std::vector<std::string>& args, MessageResponder* out) = 0;
};