#pragma once

/*
 * Empire OBS — top command bar (UI rebuild).
 *
 * The modern header: Empire brand · current profile / scene collection · live
 * CPU / FPS · an ON AIR badge with elapsed time · a clock, plus the primary
 * Go Live / Record / Studio actions. Sits across the top as the app's face.
 */

#include <obs.hpp>
#include <obs-frontend-api.h>
#include <util/platform.h>

#include <QFrame>
#include <QTimer>

class QLabel;
class QPushButton;

class EmpireCommandDock : public QFrame {
	Q_OBJECT

	QLabel *brandLabel = nullptr;
	QLabel *infoLabel = nullptr;
	QLabel *statsLabel = nullptr;
	QLabel *onAirLabel = nullptr;
	QLabel *clockLabel = nullptr;
	QPushButton *goLiveBtn = nullptr;
	QPushButton *recordBtn = nullptr;
	QPushButton *studioBtn = nullptr;

	os_cpu_usage_info_t *cpu_info = nullptr;
	QTimer timer;

	uint64_t streamStart = 0;
	uint64_t recordStart = 0;

	void Update();
	void UpdateButtons();
	void UpdateInfo();

	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

protected:
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;

public:
	EmpireCommandDock(QWidget *parent = nullptr);
	~EmpireCommandDock();
};
