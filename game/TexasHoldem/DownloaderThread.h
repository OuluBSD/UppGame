#ifndef _CardEngine_DownloaderThread_h_
#define _CardEngine_DownloaderThread_h_

#include <Core/Core.h>
#include <deque>
#include "Thread.h"

NAMESPACE_UPP

class DownloaderThread : public ThreadWrapper
{
public:
	DownloaderThread();
	virtual ~DownloaderThread();

	void QueueDownload(unsigned downloadId, const String &url, const String &filename);
	bool HasDownloadResult() const;
	bool GetDownloadResult(unsigned &downloadId, Vector<byte> &filedata);

protected:
	struct DownloadData : Moveable<DownloadData> {
		unsigned id;
		String address;
		String filename;
	};
	struct ResultData : Moveable<ResultData> {
		unsigned id;
		Vector<byte> data;
	};

	virtual void Main() override;

private:
	std::deque<DownloadData> m_downloadQueue;
	mutable Mutex m_downloadQueueMutex;

	std::deque<ResultData> m_downloadDoneQueue;
	mutable Mutex m_downloadDoneQueueMutex;
};

END_UPP_NAMESPACE

#endif
