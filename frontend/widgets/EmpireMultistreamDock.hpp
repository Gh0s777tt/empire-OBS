#pragma once

/*
 * Empire OBS — Multistream dock (Tier B #3, native UI).
 *
 * Manage up to 3 extra simultaneous RTMP destinations from a dock (instead of
 * the Scripts window). Each enabled destination gets its own rtmp_output that
 * SHARES the main stream's video/audio encoders — one encode pass, multiple
 * uploads (no extra GPU/CPU). Outputs start/stop automatically with the main
 * stream via the frontend STREAMING_STARTED / STREAMING_STOPPING events.
 */

#include <obs.hpp>

#include <QFrame>

#include <vector>

class QLineEdit;
class QCheckBox;
class QLabel;

class EmpireMultistreamDock : public QFrame {
	Q_OBJECT

	static const int NUM_DESTS = 3;

	struct DestRow {
		QCheckBox *enable = nullptr;
		QLineEdit *url = nullptr;
		QLineEdit *key = nullptr;
		QLabel *status = nullptr;
	};
	DestRow rows[NUM_DESTS];

	std::vector<OBSOutputAutoRelease> liveOutputs;
	std::vector<OBSServiceAutoRelease> liveServices;

	void Save();
	void Load();
	void StartAll();
	void StopAll();
	void SetStatus(int i, const QString &text, const char *cssColor);

	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

public:
	EmpireMultistreamDock(QWidget *parent = nullptr);
	~EmpireMultistreamDock();
};
