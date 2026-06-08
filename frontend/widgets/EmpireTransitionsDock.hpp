#pragma once

/*
 * Empire OBS — Transitions panel.
 *
 * A quick picker for the active scene transition and its duration, wired to the
 * obs_frontend transition surface. A styled companion to the stock panel.
 */

#include <obs.hpp>
#include <obs-frontend-api.h>

#include <QFrame>

class QComboBox;
class QSpinBox;

class EmpireTransitionsDock : public QFrame {
	Q_OBJECT

	QComboBox *combo = nullptr;
	QSpinBox *duration = nullptr;
	bool syncing = false;

	void RebuildList();
	void SyncCurrent();
	void SyncDuration();

	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

public:
	EmpireTransitionsDock(QWidget *parent = nullptr);
	~EmpireTransitionsDock();
};
