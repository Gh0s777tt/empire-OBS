#pragma once

/*
 * Empire OBS — Controls panel (UI rebuild, phase 5).
 *
 * The mockup's "Kontrolki" card: one-click Stream / Recording / Replay Buffer /
 * Studio Mode toggles plus Settings and Exit, wired to the obs_frontend control
 * surface. Active outputs light up red; idle buttons stay dark.
 */

#include <obs.hpp>
#include <obs-frontend-api.h>

#include <QFrame>

class QPushButton;

class EmpireControlsDock : public QFrame {
	Q_OBJECT

	QPushButton *streamBtn = nullptr;
	QPushButton *recordBtn = nullptr;
	QPushButton *replayBtn = nullptr;
	QPushButton *studioBtn = nullptr;
	QPushButton *settingsBtn = nullptr;
	QPushButton *exitBtn = nullptr;

	void UpdateStates();

	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

public:
	EmpireControlsDock(QWidget *parent = nullptr);
	~EmpireControlsDock();
};
