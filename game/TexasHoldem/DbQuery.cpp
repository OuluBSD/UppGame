#include "DbQuery.h"

NAMESPACE_UPP

SingleDbQuery::SingleDbQuery(unsigned id, const String& preparedName, Vector<Value>&& params)
	: m_id(id), m_preparedName(preparedName), m_params(std::move(params))
{
}

CompositeDbQuery::CompositeDbQuery()
{
}

String CompositeDbQuery::GetPreparedName() const
{
	if (m_currentIndex >= 0 && m_currentIndex < (int)m_list.size())
		return m_list[m_currentIndex]->GetPreparedName();
	return String();
}

const Vector<Value>& CompositeDbQuery::GetParams() const
{
	static Vector<Value> empty;
	if (m_currentIndex >= 0 && m_currentIndex < (int)m_list.size())
		return m_list[m_currentIndex]->GetParams();
	return empty;
}

bool CompositeDbQuery::RequiresResultSet() const
{
	if (m_currentIndex >= 0 && m_currentIndex < (int)m_list.size())
		return m_list[m_currentIndex]->RequiresResultSet();
	return false;
}

void CompositeDbQuery::HandleResult(Sql& sql, ServerDBCallback& cb)
{
	if (m_currentIndex >= 0 && m_currentIndex < (int)m_list.size())
		m_list[m_currentIndex]->HandleResult(sql, cb);
}

void CompositeDbQuery::HandleError(const String& error, ServerDBCallback& cb)
{
	m_errorFlag = true;
	if (m_currentIndex >= 0 && m_currentIndex < (int)m_list.size())
		m_list[m_currentIndex]->HandleError(error, cb);
}

bool CompositeDbQuery::Next()
{
	if (!m_errorFlag && m_currentIndex + 1 < (int)m_list.size()) {
		m_currentIndex++;
		return true;
	}
	return false;
}

END_UPP_NAMESPACE
