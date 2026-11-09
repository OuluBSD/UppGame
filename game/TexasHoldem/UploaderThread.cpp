#include "UploaderThread.h"
#include <Core/SSL/SSL.h>

NAMESPACE_UPP

UploaderThread::UploaderThread(UploadCallback *callback)
	: m_callback(callback)
{
}

UploaderThread::~UploaderThread()
{
	ThreadWrapper::SignalTermination();
	ThreadWrapper::Join(2000);
}

void UploaderThread::QueueUpload(const String &url, const String &user, const String &pwd, const String &filename, size_t filesize, const String &httpPost)
{
	Mutex::Lock lock(m_uploadQueueMutex);
	UploadData d;
	d.address = url;
	d.user = user;
	d.pwd = pwd;
	d.filename = filename;
	d.filesize = filesize;
	d.httpPost = httpPost;
	m_uploadQueue.push_back(std::move(d));
}

void UploaderThread::Main()
{
	String lastfile;
	while (!ThreadWrapper::ShouldTerminate()) {
		UploadData data;
		bool found = false;
		{
			Mutex::Lock lock(m_uploadQueueMutex);
			if (!m_uploadQueue.empty()) {
				data = std::move(m_uploadQueue.front());
				m_uploadQueue.pop_front();
				lastfile = data.filename;
				found = true;
			}
		}

		if (found) {
			HttpRequest client;
			client.Url(data.address);
			if (data.httpPost.GetCount()) {
				client.POST().Post(data.httpPost, LoadFile(data.filename));
			} else {
				client.PUT().Post(LoadFile(data.filename));
			}
			String response = client.Execute();
			if (client.IsSuccess()) {
				if (m_callback) m_callback->UploadCompleted(lastfile, response);
			} else {
				if (m_callback) m_callback->UploadError(lastfile, client.GetErrorDesc());
			}
		} else {
			Msleep(100);
		}
	}
}

END_UPP_NAMESPACE