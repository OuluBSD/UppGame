#ifndef _CardEngine_DbQuery_h_
#define _CardEngine_DbQuery_h_

#include <Core/Core.h>
#include <Sql/Sql.h>
#include <memory>
#include <vector>

NAMESPACE_UPP

class ServerDBCallback;

class DbQuery {
public:
	virtual ~DbQuery() {}

	virtual String GetPreparedName() const = 0;
	virtual const Vector<Value>& GetParams() const = 0;
	virtual bool RequiresResultSet() const = 0;

	virtual void HandleResult(Sql& sql, ServerDBCallback& cb) = 0;
	virtual void HandleError(const String& error, ServerDBCallback& cb) = 0;
	
	virtual bool Next() { return false; }
};

class SingleDbQuery : public DbQuery {
public:
	SingleDbQuery(unsigned id, const String& preparedName, Vector<Value>&& params);
	virtual ~SingleDbQuery() {}

	unsigned GetId() const { return m_id; }
	virtual String GetPreparedName() const override { return m_preparedName; }
	virtual const Vector<Value>& GetParams() const override { return m_params; }
	virtual bool RequiresResultSet() const override { return false; }

	virtual void HandleResult(Sql& sql, ServerDBCallback& cb) override {}
	virtual void HandleError(const String& error, ServerDBCallback& cb) override {}

private:
	unsigned m_id;
	String m_preparedName;
	Vector<Value> m_params;
};

class CompositeDbQuery : public DbQuery {
public:
	CompositeDbQuery();
	virtual ~CompositeDbQuery() {}

	void Add(std::shared_ptr<DbQuery> query) { m_list.push_back(query); }

	virtual String GetPreparedName() const override;
	virtual const Vector<Value>& GetParams() const override;
	virtual bool RequiresResultSet() const override;

	virtual void HandleResult(Sql& sql, ServerDBCallback& cb) override;
	virtual void HandleError(const String& error, ServerDBCallback& cb) override;
	
	virtual bool Next() override;

private:
	std::vector<std::shared_ptr<DbQuery>> m_list;
	int m_currentIndex = 0;
	bool m_errorFlag = false;
};

END_UPP_NAMESPACE

#endif
