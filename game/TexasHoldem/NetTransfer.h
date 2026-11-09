#ifndef _CardEngine_NetTransfer_h_
#define _CardEngine_NetTransfer_h_

#include "TransferHelper.h"
#include "Thread.h"
#include <queue>

NAMESPACE_UPP

class UploadHelper : public TransferHelper
{
public:
	virtual void InternalInit(const String &url, const String &targetFileName,
							  const String &user, const String &password,
							  size_t filesize, const String &httpPost) override {}
};

class DownloadHelper : public TransferHelper
{
public:
	virtual void InternalInit(const String &url, const String &targetFileName,
							  const String &user, const String &password,
							  size_t filesize, const String &httpPost) override {}
};

class DownloaderThread : public ThreadWrapper
{
public:
	DownloaderThread();
	virtual ~DownloaderThread();

	void QueueDownload(unsigned downloadId, const String &url, const String &filename);
	bool HasDownloadResult() const;
	bool GetDownloadResult(unsigned &downloadId, Vector<byte> &filedata);

protected:
	struct DownloadData {
		unsigned id;
		String address;
		String filename;
	};
	struct ResultData {
		unsigned id;
		Vector<byte> data;
	};

	virtual void Main() override;

private:
	std::queue<DownloadData> m_downloadQueue;
	mutable Mutex m_downloadQueueMutex;

	std::queue<ResultData> m_downloadDoneQueue;
	mutable Mutex m_downloadDoneQueueMutex;

	std::shared_ptr<DownloadHelper> m_downloadHelper;
	bool m_downloadInProgress;
};

class UploadCallback;

class UploaderThread : public ThreadWrapper
{
public:
	UploaderThread(UploadCallback *callback = nullptr);
	virtual ~UploaderThread();

	void QueueUpload(const String &url, const String &user, const String &pwd, const String &filename, size_t filesize, const String &httpPost = "");

protected:
	struct UploadData {
		String address;
		String user;
		String pwd;
		String filename;
		size_t filesize;
		String httpPost;
	};

	virtual void Main() override;

private:
	std::queue<UploadData> m_uploadQueue;
	mutable Mutex m_uploadQueueMutex;

	std::shared_ptr<UploadHelper> m_uploadHelper;
	bool m_uploadInProgress;
	UploadCallback *m_callback;
};

END_UPP_NAMESPACE

#endif
