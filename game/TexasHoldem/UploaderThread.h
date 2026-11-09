#ifndef _CardEngine_UploaderThread_h_
#define _CardEngine_UploaderThread_h_

#include <Core/Core.h>
#include <deque>
#include "Thread.h"

NAMESPACE_UPP

class UploadCallback {
public:
	virtual ~UploadCallback() {}
	virtual void UploadCompleted(const String &filename, const String &msg) = 0;
	virtual void UploadError(const String &filename, const String &msg) = 0;
};

class UploaderThread : public ThreadWrapper
{
public:
	UploaderThread(UploadCallback *callback = nullptr);
	virtual ~UploaderThread();

	void QueueUpload(const String &url, const String &user, const String &pwd, const String &filename, size_t filesize, const String &httpPost = "");

protected:
	struct UploadData : Moveable<UploadData> {
		String address;
		String user;
		String pwd;
		String filename;
		size_t filesize;
		String httpPost;
	};

	virtual void Main() override;

private:
	std::deque<UploadData> m_uploadQueue;
	mutable Mutex m_uploadQueueMutex;

	UploadCallback *m_callback;
};

END_UPP_NAMESPACE

#endif
