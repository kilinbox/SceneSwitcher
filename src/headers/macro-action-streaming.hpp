#pragma once
#include "macro-action-edit.hpp"

#include <QDoubleSpinBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <chrono>

enum class StreamAction {
	STOP,
	START,
};

class MacroActionStream : public MacroAction {
public:
	MacroActionStream(Macro *m) : MacroAction(m) {}
	bool PerformAction();
	void LogAction();
	bool Save(obs_data_t *obj);
	bool Load(obs_data_t *obj);
	std::string GetId() { return id; };
	static std::shared_ptr<MacroAction> Create(Macro *m)
	{
		return std::make_shared<MacroActionStream>(m);
	}

	StreamAction _action = StreamAction::STOP;

private:
	bool CooldownDurationReached();
	static std::chrono::high_resolution_clock::time_point s_lastAttempt;

	static bool _registered;
	static const std::string id;
};

class MacroActionStreamEdit : public QWidget {
	Q_OBJECT

public:
	MacroActionStreamEdit(
		QWidget *parent,
		std::shared_ptr<MacroActionStream> entryData = nullptr);
	void UpdateEntryData();
	static QWidget *Create(QWidget *parent,
			       std::shared_ptr<MacroAction> action)
	{
		return new MacroActionStreamEdit(
			parent,
			std::dynamic_pointer_cast<MacroActionStream>(action));
	}

private slots:
	void ActionChanged(int value);

protected:
	QComboBox *_actions;
	std::shared_ptr<MacroActionStream> _entryData;

private:
	QHBoxLayout *_mainLayout;
	bool _loading = true;
};
