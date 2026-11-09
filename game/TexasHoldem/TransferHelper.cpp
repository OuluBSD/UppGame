#include "TransferHelper.h"

NAMESPACE_UPP

struct TransferData {};

TransferHelper::TransferHelper() : m_data(std::make_shared<TransferData>()) {}
TransferHelper::~TransferHelper() {}

void TransferHelper::Init(const String &url, const String &targetFileName, const String &user, const String &password, size_t filesize, const String &httpPost)
{
	InternalInit(url, targetFileName, user, password, filesize, httpPost);
}

bool TransferHelper::Process() { return true; }
void TransferHelper::Cleanup() {}
String TransferHelper::ResetLastMessage() { return ""; }
std::shared_ptr<TransferData> TransferHelper::GetData() { return m_data; }

END_UPP_NAMESPACE
