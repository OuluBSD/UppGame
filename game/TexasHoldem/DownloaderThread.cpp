#include "DownloaderThread.h"
#include <Core/SSL/SSL.h>

NAMESPACE_UPP

DownloaderThread::DownloaderThread()
{
}

DownloaderThread::~DownloaderThread()
{
	ThreadWrapper::SignalTermination();
	ThreadWrapper::Join(2000);
}

void DownloaderThread::QueueDownload(unsigned downloadId, const String &url, const String &filename)
{
	Mutex::Lock lock(m_downloadQueueMutex);
	DownloadData d;
	d.id = downloadId;
	d.address = url;
	d.filename = filename;
	m_downloadQueue.push_back(std::move(d));
}

bool DownloaderThread::HasDownloadResult() const
{
	Mutex::Lock lock(m_downloadDoneQueueMutex);
	return !m_downloadDoneQueue.empty();
}

bool DownloaderThread::GetDownloadResult(unsigned &downloadId, Vector<byte> &filedata)
{
	Mutex::Lock lock(m_downloadDoneQueueMutex);
	if (!m_downloadDoneQueue.empty()) {
		ResultData d = std::move(m_downloadDoneQueue.front());
		downloadId = d.id;
		filedata = std::move(d.data);
		m_downloadDoneQueue.pop_front();
		return true;
	}
	return false;
}

void DownloaderThread::Main()
{
	while (!ThreadWrapper::ShouldTerminate()) {
		DownloadData cur;
		bool found = false;
		{
			Mutex::Lock lock(m_downloadQueueMutex);
			if (!m_downloadQueue.empty()) {
				cur = std::move(m_downloadQueue.front());
				m_downloadQueue.pop_front();
				found = true;
			}
		}

		if (found) {
			HttpRequest client;
			client.Url(cur.address);
			String content = client.Execute();
			if (client.IsSuccess()) {
				Mutex::Lock lock(m_downloadDoneQueueMutex);
				ResultData res;
				res.id = cur.id;
				res.data.SetCount(content.GetCount());
				memcpy(res.data.Begin(), ~content, content.GetCount());
				m_downloadDoneQueue.push_back(std::move(res));
			}
		} else {
			Msleep(100);
		}
	}
}

END_UPP_NAMESPACE