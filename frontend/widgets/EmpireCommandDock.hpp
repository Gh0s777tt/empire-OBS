#pragma once

/*
 * Empire OBS — Command Center dock (UI rebuild, phase 1).
 *
 * A sleek horizontal "command bar": LIVE / REC status + elapsed timer, live
 * vitals (CPU / FPS / dropped / bitrate) and big primary actions
 * (Go Live · Record · Studio). Meant to sit across the top as the modern face.
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

	QLabel *statusLabel = nullptr;
	QLabel *statsLabel = nullptr;
	QPushButton *goLiveBtn = nullptr;
	QPushButton *recordBtn = nullptr;
	QPushButton *studioBtn = nullptr;

	os_cpu_usage_info_t *cpu_info = nullptr;
	QTimer timer;

	uint64_t streamStart = 0;
	uint64_t recordStart = 0;
	int first_total = 0;
	int first_dropped = 0;
	uint64_t lastBytes = 0;
	uint64_t lastBytesTime = 0;

	void Update();
	void UpdateButtons();

	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

protected:
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;

public:
	EmpireCommandDock(QWidget *parent = nullptr);
	~EmpireCommandDock();
};
