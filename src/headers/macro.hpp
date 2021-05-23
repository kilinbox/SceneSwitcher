#pragma once
#include <string>
#include <deque>
#include <memory>
#include <map>
#include <obs.hpp>
#include <obs-module.h>
#include <QString>

constexpr auto macro_func = 10;
constexpr auto default_priority_10 = macro_func;

constexpr auto logic_root_offset = 100;

enum class LogicType {
	ROOT_NONE = 0,
	ROOT_NOT,
	ROOT_LAST,
	// leave some space for potential expansion
	NONE = 100,
	AND,
	OR,
	AND_NOT,
	OR_NOT,
	LAST,
};

static inline bool isRootLogicType(LogicType l)
{
	return static_cast<int>(l) < logic_root_offset;
}

struct LogicTypeInfo {
	std::string _name;
};

class MacroCondition {
public:
	virtual bool CheckCondition() = 0;
	virtual bool Save(obs_data_t *obj) = 0;
	virtual bool Load(obs_data_t *obj) = 0;
	virtual std::string GetId() = 0;
	LogicType GetLogicType() { return _logic; }
	void SetLogicType(LogicType logic) { _logic = logic; }

	static const std::map<LogicType, LogicTypeInfo> logicTypes;

private:
	LogicType _logic;
};

class MacroAction {
public:
	virtual bool PerformAction() = 0;
	virtual bool Save(obs_data_t *obj) = 0;
	virtual bool Load(obs_data_t *obj) = 0;
	virtual std::string GetId() = 0;
	virtual void LogAction();
};

class Macro {

public:
	Macro(std::string name = "");
	virtual ~Macro();

	bool CeckMatch();
	bool PerformAction();
	bool Matched() { return _matched; }
	std::string Name() { return _name; }
	void SetName(const std::string &name) { _name = name; }
	void SetPaused(bool pause = true) { _paused = pause; }
	bool Paused() { return _paused; }
	std::deque<std::shared_ptr<MacroCondition>> &Conditions()
	{
		return _conditions;
	}
	std::deque<std::shared_ptr<MacroAction>> &Actions() { return _actions; }

	bool Save(obs_data_t *obj);
	bool Load(obs_data_t *obj);

	// Helper function for plugin state condition regarding scene change
	bool SwitchesScene();

private:
	std::string _name = "";
	std::deque<std::shared_ptr<MacroCondition>> _conditions;
	std::deque<std::shared_ptr<MacroAction>> _actions;
	bool _matched = false;
	bool _paused = false;
};

Macro *GetMacroByName(const char *name);
Macro *GetMacroByQString(const QString &name);
