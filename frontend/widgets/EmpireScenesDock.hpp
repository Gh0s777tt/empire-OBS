#pragma once

/*
 * Empire OBS — Scenes switcher (UI rebuild, phase 2).
 *
 * A modern card-based scene switcher: each scene is a rounded card you click to
 * cut to it; the active scene glows red. A cleaner face for the stock list.
 */

#include <obs.hpp>
#include <obs-frontend-api.h>

#include <QFrame>

class QVBoxLayout;

class EmpireScenesDock : public QFrame {
	Q_OBJECT

	QVBoxLayout *cardLayout = nullptr;

	void RebuildScenes();
	void UpdateActive();
	void SceneCardMenu(const QString &sceneName, const QPoint &globalPos);

	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

public:
	EmpireScenesDock(QWidget *parent = nullptr);
	~EmpireScenesDock();
};
