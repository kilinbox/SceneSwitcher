#include "headers/macro-action-scene-switch.hpp"
#include "headers/advanced-scene-switcher.hpp"
#include "headers/utility.hpp"

using namespace std::chrono_literals;

const std::string MacroActionSwitchScene::id = "scene_switch";

bool MacroActionSwitchScene::_registered = MacroActionFactory::Register(
	MacroActionSwitchScene::id,
	{MacroActionSwitchScene::Create, MacroActionSwitchSceneEdit::Create,
	 "AdvSceneSwitcher.action.switchScene"});

void waitForTransitionChange(OBSWeakSource &transition)
{
	const auto time = 100ms;
	obs_source_t *source = obs_weak_source_get_source(transition);
	std::unique_lock<std::mutex> lock(switcher->m);
	bool stillTransitioning = true;
	while (stillTransitioning && !switcher->abortMacroWait) {
		switcher->macroTransitionCv.wait_for(lock, time);
		float t = obs_transition_get_time(source);
		stillTransitioning = t < 1.0f && t > 0.0f;
	}
	obs_source_release(source);
}

void waitForTransitionChangeFixedDuration(int duration)
{
	duration += 200; // It seems to be necessary to add a small buffer
	auto time = std::chrono::high_resolution_clock::now() +
		    std::chrono::milliseconds(duration);

	switcher->abortMacroWait = false;
	std::unique_lock<std::mutex> lock(switcher->m);
	while (!switcher->abortMacroWait) {
		if (switcher->macroTransitionCv.wait_until(lock, time) ==
		    std::cv_status::timeout) {
			break;
		}
	}
}

int getTransitionOverrideDuration(OBSWeakSource &scene)
{
	int duration = 0;
	obs_source_t *source = obs_weak_source_get_source(scene);
	obs_data_t *data = obs_source_get_private_settings(source);
	auto name = obs_data_get_string(data, "transition");
	if (strlen(name) != 0) {
		duration = obs_data_get_int(data, "transition_duration");
	}
	obs_data_release(data);
	obs_source_release(source);
	return duration;
}

bool isUsingFixedLengthTransition(OBSWeakSource &transition)
{
	obs_source_t *source = obs_weak_source_get_source(transition);
	bool ret = obs_transition_fixed(source);
	obs_source_release(source);
	return ret;
}

OBSWeakSource getOverrideTransition(OBSWeakSource &scene)
{
	OBSWeakSource transition;
	obs_source_t *source = obs_weak_source_get_source(scene);
	obs_data_t *data = obs_source_get_private_settings(source);
	transition = GetWeakTransitionByName(
		obs_data_get_string(data, "transition"));
	obs_data_release(data);
	obs_source_release(source);
	return transition;
}

int getExpectedTransitionDuration(OBSWeakSource &scene, OBSWeakSource &t,
				  double duration)
{
	OBSWeakSource transition = t;
	if (!switcher->transitionOverrideOverride) {
		auto overrideTransition = getOverrideTransition(scene);
		if (overrideTransition) {
			transition = overrideTransition;
			if (!isUsingFixedLengthTransition(transition)) {
				return getTransitionOverrideDuration(scene);
			}
		}
	}
	if (isUsingFixedLengthTransition(transition)) {
		return -1; // no API is available to access the fixed duration
	}
	if (duration != 0) {
		return duration * 1000;
	}
	return obs_frontend_get_transition_duration();
}

bool MacroActionSwitchScene::PerformAction()
{
	auto scene = _scene.GetScene();
	auto transition = _transition.GetTransition();
	switchScene({scene, transition, (int)(_duration.seconds * 1000)},
		    obs_frontend_preview_program_mode_active());
	if (_blockUntilTransitionDone && scene) {
		const int expectedTransitionDuration =
			getExpectedTransitionDuration(scene, transition,
						      _duration.seconds);
		if (expectedTransitionDuration < 0) {
			waitForTransitionChange(transition);
		} else {
			waitForTransitionChangeFixedDuration(
				expectedTransitionDuration);
		}
		return !switcher->abortMacroWait;
	}
	return true;
}

void MacroActionSwitchScene::LogAction()
{
	auto t = _scene.GetType();
	auto sceneName = GetWeakSourceName(_scene.GetScene(false));
	switch (t) {
	case SceneSelectionType::SCENE:
		vblog(LOG_INFO, "switch to scene '%s'",
		      _scene.ToString().c_str());
		break;
	case SceneSelectionType::GROUP:
		vblog(LOG_INFO, "switch to scene '%s' (scene group '%s')",
		      sceneName.c_str(), _scene.ToString().c_str());
		break;
	case SceneSelectionType::PREVIOUS:
		vblog(LOG_INFO, "switch to previous scene '%s'",
		      sceneName.c_str());
		break;
	default:
		break;
	}
}

bool MacroActionSwitchScene::Save(obs_data_t *obj)
{
	MacroAction::Save(obj);
	_scene.Save(obj);
	_transition.Save(obj);
	_duration.Save(obj);
	obs_data_set_bool(obj, "blockUntilTransitionDone",
			  _blockUntilTransitionDone);
	return true;
}

bool MacroActionSwitchScene::Load(obs_data_t *obj)
{
	// Convert old data format
	// TODO: Remove in future version
	if (obs_data_has_user_value(obj, "targetType")) {
		auto sceneName = obs_data_get_string(obj, "target");
		obs_data_set_string(obj, "scene", sceneName);
		auto transitionName = obs_data_get_string(obj, "transition");
		bool usePreviousScene =
			strcmp(sceneName, previous_scene_name) == 0;
		bool useCurrentTransition =
			strcmp(transitionName, current_transition_name) == 0;
		obs_data_set_int(
			obj, "sceneType",
			(usePreviousScene)
				? static_cast<int>(SceneSelectionType::PREVIOUS)
				: obs_data_get_int(obj, "targetType"));
		obs_data_set_int(
			obj, "transitionType",
			(useCurrentTransition)
				? static_cast<int>(
					  TransitionSelectionType::CURRENT)
				: static_cast<int>(
					  TransitionSelectionType::TRANSITION));
	}

	MacroAction::Load(obj);
	_scene.Load(obj);
	_transition.Load(obj);
	_duration.Load(obj);
	_blockUntilTransitionDone =
		obs_data_get_bool(obj, "blockUntilTransitionDone");
	return true;
}

std::string MacroActionSwitchScene::GetShortDesc()
{
	return _scene.ToString();
}

MacroActionSwitchSceneEdit::MacroActionSwitchSceneEdit(
	QWidget *parent, std::shared_ptr<MacroActionSwitchScene> entryData)
	: QWidget(parent)
{
	_scenes = new SceneSelectionWidget(window(), true, true);
	_transitions = new TransitionSelectionWidget(this);
	_duration = new DurationSelection(parent, false);
	_blockUntilTransitionDone = new QCheckBox(obs_module_text(
		"AdvSceneSwitcher.action.scene.blockUntilTransitionDone"));

	_duration->SpinBox()->setSpecialValueText("-");

	QWidget::connect(_scenes, SIGNAL(SceneChanged(const SceneSelection &)),
			 this, SLOT(SceneChanged(const SceneSelection &)));
	QWidget::connect(_transitions,
			 SIGNAL(TransitionChanged(const TransitionSelection &)),
			 this,
			 SLOT(TransitionChanged(const TransitionSelection &)));
	QWidget::connect(_duration, SIGNAL(DurationChanged(double)), this,
			 SLOT(DurationChanged(double)));
	QWidget::connect(_blockUntilTransitionDone, SIGNAL(stateChanged(int)),
			 this, SLOT(BlockUntilTransitionDoneChanged(int)));

	QHBoxLayout *entryLayout = new QHBoxLayout;
	std::unordered_map<std::string, QWidget *> widgetPlaceholders = {
		{"{{scenes}}", _scenes},
		{"{{transitions}}", _transitions},
		{"{{duration}}", _duration},
		{"{{blockUntilTransitionDone}}", _blockUntilTransitionDone},
	};
	placeWidgets(obs_module_text("AdvSceneSwitcher.action.scene.entry"),
		     entryLayout, widgetPlaceholders);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(entryLayout);
	mainLayout->addWidget(_blockUntilTransitionDone);
	setLayout(mainLayout);

	_entryData = entryData;
	_scenes->SetScene(_entryData->_scene);
	_transitions->SetTransition(_entryData->_transition);
	_duration->SetDuration(_entryData->_duration);
	_blockUntilTransitionDone->setChecked(
		_entryData->_blockUntilTransitionDone);
	_loading = false;
}

void MacroActionSwitchSceneEdit::DurationChanged(double seconds)
{
	if (_loading || !_entryData) {
		return;
	}

	std::lock_guard<std::mutex> lock(switcher->m);
	_entryData->_duration.seconds = seconds;
}

void MacroActionSwitchSceneEdit::BlockUntilTransitionDoneChanged(int state)
{
	if (_loading || !_entryData) {
		return;
	}

	std::lock_guard<std::mutex> lock(switcher->m);
	_entryData->_blockUntilTransitionDone = state;
}

void MacroActionSwitchSceneEdit::SceneChanged(const SceneSelection &s)
{
	if (_loading || !_entryData) {
		return;
	}

	std::lock_guard<std::mutex> lock(switcher->m);
	_entryData->_scene = s;
	emit HeaderInfoChanged(
		QString::fromStdString(_entryData->GetShortDesc()));
}

void MacroActionSwitchSceneEdit::TransitionChanged(const TransitionSelection &t)
{
	if (_loading || !_entryData) {
		return;
	}

	std::lock_guard<std::mutex> lock(switcher->m);
	_entryData->_transition = t;
}
