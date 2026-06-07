#pragma once

/*
 * Empire OBS — Audio panel (UI rebuild, phase 3).
 *
 * A clean per-source audio strip: name, a dB-mapped volume slider (obs_fader)
 * and a mute toggle. A modern companion to the stock mixer.
 */

#include <obs.hpp>
#include <obs-frontend-api.h>

#include <QFrame>

#include <vector>

class QVBoxLayout;

class EmpireAudioDock : public QFrame {
	Q_OBJECT

	QVBoxLayout *rowLayout = nullptr;
	std::vector<obs_fader_t *> faders;

	void Rebuild();
	void AddSourceRow(obs_source_t *src);

	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

public:
	EmpireAudioDock(QWidget *parent = nullptr);
	~EmpireAudioDock();
};
