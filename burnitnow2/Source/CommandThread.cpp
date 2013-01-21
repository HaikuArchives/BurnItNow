/*
 * Copyright 2010, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CommandThread.h"

#include "CommandPipe.h"

#include <AutoLocker.h>


class CommandReader : public BPrivate::BCommandPipe::LineReader {
public:
	CommandReader(BInvoker* invoker)
	:
	fInvoker(invoker) {}


	virtual bool IsCanceled()
	{
		// TODO Handle cancel
		return false;
	}

	virtual status_t ReadLine(const BString& line)
	{
		if (fInvoker == NULL)
			return B_OK;

		BMessage messageCopy(*fInvoker->Message());

		BString lineCopy(line);
		lineCopy.RemoveLast("\n");

		messageCopy.AddString("line", lineCopy);

		fInvoker->Invoke(&messageCopy);

		return B_OK;
	}

private:
	BInvoker* fInvoker;
};


CommandThread::CommandThread(BObjectList<BString>* argList, BInvoker* invoker)
	:
	fArgumentList(argList),
	fInvoker(invoker)
{
	if (fArgumentList == NULL)
		fArgumentList = new BObjectList<BString>(5, true);
}


CommandThread::~CommandThread()
{
	delete fInvoker;
	delete fArgumentList;
}


BObjectList<BString>* CommandThread::Arguments()
{
	AutoLocker<CommandThread> locker(this);

	return fArgumentList;
}


#pragma mark -- Public Methods --


void CommandThread::SetArguments(BObjectList<BString>* argList)
{
	AutoLocker<CommandThread> locker(this);

	delete fArgumentList;
	fArgumentList = argList;
}


CommandThread* CommandThread::AddArgument(const char* argument)
{
	AutoLocker<CommandThread> locker(this);

	fArgumentList->AddItem(new BString(argument));

	return this;
}


BInvoker* CommandThread::Invoker()
{
	AutoLocker<CommandThread> locker(this);

	return fInvoker;
}


void CommandThread::SetInvoker(BInvoker* invoker)
{
	AutoLocker<CommandThread> locker(this);

	delete fInvoker;

	fInvoker = invoker;
}


status_t CommandThread::Run()
{
	AutoLocker<CommandThread> locker(this);

	//TODO Check if thread is already running

	fThread = spawn_thread(CommandThread::_Thread, "command thread", B_NORMAL_PRIORITY, this);
	if (fThread < B_OK)
		return B_ERROR;

	if (resume_thread(fThread) != B_OK)
		return B_ERROR;

	return B_OK;
}


status_t CommandThread::Stop()
{
	AutoLocker<CommandThread> locker(this);

	return B_ERROR;
}


status_t CommandThread::Wait()
{
	AutoLocker<CommandThread> locker(this);

	status_t status;

	wait_for_thread(fThread, &status);

	return status;
}


#pragma mark -- Private Thread Functions --


int32 CommandThread::_Thread(void* data)
{
	on_exit_thread(CommandThread::_ThreadExit, data);

	CommandThread* commandThread = static_cast<CommandThread*>(data);

	if (commandThread == NULL)
		return B_ERROR;

	// TODO acquire autolock

	BCommandPipe pipe;

	BObjectList<BString>* args = commandThread->Arguments();

	for (int32 x = 0; x < args->CountItems(); x++)
		pipe << *args->ItemAt(x);

	FILE* stdOutAndErrPipe = NULL;

	thread_id pipeThread = pipe.PipeInto(&stdOutAndErrPipe);

	if (pipeThread < B_OK)
		return B_ERROR;

	BPrivate::BCommandPipe::LineReader* reader = new CommandReader(commandThread->Invoker());

	if (pipe.ReadLines(stdOutAndErrPipe, reader) != B_OK) {
		kill_thread(pipeThread);
		status_t exitval;
		wait_for_thread(pipeThread, &exitval);
		return B_ERROR;
	}

	return B_OK;
}


void CommandThread::_ThreadExit(void* data)
{
	CommandThread* commandThread = static_cast<CommandThread*>(data);

	if (commandThread == NULL)
		return;

	BInvoker* invoker = commandThread->Invoker();

	if (invoker == NULL)
		return;

	BMessage copy(*invoker->Message());

	// TODO adjust this based on the actual command exit code
	copy.AddInt32("thread_exit", 0);

	invoker->Invoke(&copy);
}
