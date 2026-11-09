#ifndef _CardEngine_TransferHelper_h_
#define _CardEngine_TransferHelper_h_

#include <memory>
#include <Core/Core.h>

NAMESPACE_UPP

struct TransferData;

class TransferHelper
{
public:
	TransferHelper();
	virtual ~TransferHelper();

	void Init(const String &url, const String &targetFileName,
			  const String &user = "", const String &password = "",
			  size_t filesize = 0, const String &httpPost = "");

	bool Process();
	void Cleanup();
	String ResetLastMessage();

protected:
	std::shared_ptr<TransferData> GetData();

	virtual void InternalInit(const String &url, const String &targetFileName,
							  const String &user, const String &password,
							  size_t filesize, const String &httpPost) = 0;

private:
	std::shared_ptr<TransferData> m_data;
};

END_UPP_NAMESPACE

#endif
