#pragma once

/*
 * Empire OBS — Vertical 9:16 dock.
 *
 * Hosts a private 1080x1920 libobs canvas (obs_canvas API) and renders a live
 * preview of it into an OBSQTDisplay. For now the vertical canvas mirrors the
 * current program scene (channel 0), so streamers get an instant 9:16 view of
 * their content. A dedicated vertical scene + a vertical recording/stream
 * output are the next iterations.
 */

#include <obs.hpp>
#include <obs-frontend-api.h>

#include <QFrame>

class OBSQTDisplay;

class EmpireVerticalDock : public QFrame {
	Q_OBJECT

	OBSQTDisplay *display = nullptr;
	obs_canvas_t *canvas = nullptr;

	void SyncToCurrentScene();

	static void RenderVertical(void *data, uint32_t cx, uint32_t cy);
	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

public:
	EmpireVerticalDock(QWidget *parent = nullptr);
	~EmpireVerticalDock();
};
