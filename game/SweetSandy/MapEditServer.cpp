#include "SweetSandy.h"

NAMESPACE_UPP

void MapEditServer::Start(int port)
{
	Stop();
	thread_running = true;
	thread.Run([this, port] { ThreadLoop(port); });
}

void MapEditServer::Stop()
{
	if(thread_running) {
		thread_running = false;
		if(thread.IsOpen())
			thread.Wait();
	}
}

void MapEditServer::ThreadLoop(int port)
{
	TcpSocket listener;
	if(!listener.Listen(port, 5, false)) {
		LOG("MapEditServer: failed to listen on 127.0.0.1:" << port << ": " << listener.GetErrorDesc());
		thread_running = false;
		return;
	}
	LOG("MapEditServer: listening on 127.0.0.1:" << port);

	while(thread_running) {
		TcpSocket client;
		client.Timeout(5); // ms - short accept poll, same idiom uppsrc/MCP/Server.cpp's own Loop() uses
		if(!client.Accept(listener))
			continue; // timeout or transient error - loop, re-check thread_running

		client.Timeout(3000); // a connected client sending one short line should do so promptly
		String line = TrimBoth(client.GetLine());
		if(line.IsEmpty()) {
			client.Close();
			continue;
		}

		{
			Mutex::Lock __(lock);
			pending_command = line;
			has_command = true;
			has_response = false;
		}
		// Wait for the GUI thread to pick this up and post a response -
		// same short-sleep polling convention as WorldChunkGenWorker's
		// own worker loop (Task 9).
		while(thread_running) {
			bool ready;
			{
				Mutex::Lock __(lock);
				ready = has_response;
			}
			if(ready)
				break;
			Sleep(5);
		}
		String response;
		{
			Mutex::Lock __(lock);
			response = pending_response;
			has_response = false;
		}
		response.Cat('\n');
		client.PutAll(response);
		client.Close();
	}
}

bool MapEditServer::PollCommand(String& out_command)
{
	Mutex::Lock __(lock);
	if(!has_command)
		return false;
	out_command = pending_command;
	has_command = false;
	return true;
}

void MapEditServer::PostResponse(const String& response)
{
	Mutex::Lock __(lock);
	pending_response = response;
	has_response = true;
}

END_UPP_NAMESPACE
