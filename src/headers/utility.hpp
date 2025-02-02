#pragma once
#include <QString>
#include <QLayout>
#include <QComboBox>
#include <QMetaObject>
#include <QListWidget>
#include <QPushButton>
#include <QColor>
#include <obs.hpp>
#include <obs-frontend-api.h>
#include <deque>
#include <unordered_map>
#include "scene-group.hpp"

class SceneSelection;

bool WeakSourceValid(obs_weak_source_t *ws);
std::string GetWeakSourceName(obs_weak_source_t *weak_source);
OBSWeakSource GetWeakSourceByName(const char *name);
OBSWeakSource GetWeakSourceByQString(const QString &name);
OBSWeakSource GetWeakTransitionByName(const char *transitionName);
OBSWeakSource GetWeakTransitionByQString(const QString &name);
OBSWeakSource GetWeakFilterByName(OBSWeakSource source, const char *name);
OBSWeakSource GetWeakFilterByQString(OBSWeakSource source, const QString &name);
bool compareIgnoringLineEnding(QString &s1, QString &s2);
std::string getSourceSettings(OBSWeakSource ws);
void setSourceSettings(obs_source_t *s, const std::string &settings);
bool compareSourceSettings(const OBSWeakSource &source,
			   const std::string &settings, bool regex);
std::vector<obs_scene_item *> getSceneItemsWithName(obs_scene_t *scene,
						    std::string &name);
std::string getDataFilePath(const std::string &file);
bool matchJson(const std::string &json1, const std::string &json2,
	       bool useRegex);
QString formatJsonString(std::string);
QString formatJsonString(QString);
QString escapeForRegex(QString &s);
void loadTransformState(obs_data_t *obj, struct obs_transform_info &info,
			struct obs_sceneitem_crop &crop);
bool saveTransformState(obs_data_t *obj, struct obs_transform_info &info,
			struct obs_sceneitem_crop &crop);
std::string getSceneItemTransform(obs_scene_item *item);
void placeWidgets(std::string text, QBoxLayout *layout,
		  std::unordered_map<std::string, QWidget *> placeholders,
		  bool addStretch = true);
void deleteLayoutItemWidget(QLayoutItem *item);
void clearLayout(QLayout *layout, int afterIdx = 0);
void setLayoutVisible(QLayout *layout, bool visible);
QMetaObject::Connection PulseWidget(QWidget *widget, QColor startColor,
				    QColor endColor = QColor(0, 0, 0, 0),
				    bool once = false);
void listAddClicked(QListWidget *list, QWidget *newWidget,
		    QPushButton *addButton = nullptr,
		    QMetaObject::Connection *addHighlight = nullptr);
bool listMoveUp(QListWidget *list);
bool listMoveDown(QListWidget *list);
void setHeightToContentHeight(QListWidget *list);
bool DisplayMessage(const QString &msg, bool question = false);
void DisplayTrayMessage(const QString &title, const QString &msg);
void addSelectionEntry(QComboBox *sel, const char *description,
		       bool selectable = false, const char *tooltip = "");
void populateTransitionSelection(QComboBox *sel, bool addCurrent = true,
				 bool addAny = false);
void populateWindowSelection(QComboBox *sel, bool addSelect = true);
void populateAudioSelection(QComboBox *sel, bool addSelect = true);
void populateVideoSelection(QComboBox *sel, bool addMainOutput = false,
			    bool addScenes = false, bool addSelect = true);
void populateMediaSelection(QComboBox *sel, bool addSelect = true);
void populateProcessSelection(QComboBox *sel, bool addSelect = true);
void populateSourceSelection(QComboBox *list, bool addSelect = true);
void populateSceneSelection(QComboBox *sel, bool addPrevious = false,
			    bool addCurrent = false, bool addAny = false,
			    bool addSceneGroup = false,
			    std::deque<SceneGroup> *sceneGroups = nullptr,
			    bool addSelect = true, std::string selectText = "",
			    bool selectable = false);
void populateSourcesWithFilterSelection(QComboBox *list);
void populateFilterSelection(QComboBox *list,
			     OBSWeakSource weakSource = nullptr);
void populateSceneItemSelection(QComboBox *list,
				OBSWeakSource sceneWeakSource = nullptr);
void populateSceneItemSelection(QComboBox *list, SceneSelection &s);
void populateSourceGroupSelection(QComboBox *list);
void populateProfileSelection(QComboBox *list);
bool windowPosValid(QPoint pos);
bool doubleEquals(double left, double right, double epsilon);
