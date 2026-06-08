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
#include <QTimer>
#include <QWidget>

#include <atomic>
#include <vector>

class QVBoxLayout;

/* A thin peak-level bar. The volmeter callback (audio thread) stores the
 * normalized peak; a GUI-thread timer applies decay and triggers a repaint. */
class EmpireLevelBar : public QWidget {
public:
	explicit EmpireLevelBar(QWidget *parent = nullptr);
	void setMagnitude(float m01) { magnitude.store(m01, std::memory_order_relaxed); }
	void refresh();

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	std::atomic<float> magnitude{0.0f};
	float displayed = 0.0f;
};

class EmpireAudioDock : public QFrame {
	Q_OBJECT

	QVBoxLayout *rowLayout = nullptr;
	std::vector<obs_fader_t *> faders;
	std::vector<obs_volmeter_t *> volmeters;
	std::vector<EmpireLevelBar *> meters;
	QTimer meterTimer;

	void Rebuild();
	void AddSourceRow(obs_source_t *src);

	static void VolmeterCallback(void *param, const float magnitude[MAX_AUDIO_CHANNELS],
				     const float peak[MAX_AUDIO_CHANNELS], const float input_peak[MAX_AUDIO_CHANNELS]);

	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

public:
	EmpireAudioDock(QWidget *parent = nullptr);
	~EmpireAudioDock();
};
