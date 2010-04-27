#pragma once

struct MessageResponder
{
	/**
	 * Respond to the message.
     *
     * The implementation will delete itself.
     *
	 * @param error The error code, or zero if successful.
	 * @param args	The list of output strings.
	 */
	virtual void run(int error, const std::vector<std::string>& args) = 0;
};