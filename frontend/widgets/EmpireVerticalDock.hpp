#pragma once

/*
 * Empire OBS — Vertical 9:16 dock.
 *
 * Hosts a private 1080x1920 libobs canvas (obs_canvas API) and renders a live
 * preview of it into an OBSQTDisplay. The canvas owns a dedicated vertical
 * scene that mirrors the current program scene as a single item, scaled to
 * FILL (crop to cover 9:16) or FIT (letterbox) the frame — toggled live.
 * The vertical canvas can be recorded to a file and streamed to an RTMP
 * destination, independently of (and alongside) the main 16:9 output.
 */

#include <obs.hpp>
#include <obs-frontend-api.h>

#include <QFrame>

class OBSQTDisplay;
class QPushButton;
class QLineEdit;

class EmpireVerticalDock : public QFrame {
	Q_OBJECT

	OBSQTDisplay *display = nullptr;
	QPushButton *modeButton = nullptr;
	QPushButton *recordButton = nullptr;
	QPushButton *streamButton = nullptr;
	QLineEdit *urlEdit = nullptr;
	QLineEdit *keyEdit = nullptr;

	obs_canvas_t *canvas = nullptr;
	obs_scene_t *vScene = nullptr;         /* borrowed — owned by the canvas */
	obs_sceneitem_t *mirrorItem = nullptr; /* the program-mirror item in vScene */
	bool fillMode = true;                  /* true = Fill/crop, false = Fit/letterbox */

	OBSOutputAutoRelease recordOutput;
	OBSEncoderAutoRelease recordVEnc;
	OBSEncoderAutoRelease recordAEnc;
	bool recording = false;

	OBSOutputAutoRelease streamOutput;
	OBSEncoderAutoRelease streamVEnc;
	OBSEncoderAutoRelease streamAEnc;
	OBSServiceAutoRelease streamService;
	bool streaming = false;

	void SyncToCurrentScene();
	void ApplyFraming();
	void UpdateModeButton();
	void ToggleRecording();
	void StartRecording();
	void UpdateRecordButton();
	void ToggleStreaming();
	void StartStreaming();
	void UpdateStreamButton();
	void LoadConfig();
	void SaveConfig();

	static void RenderVertical(void *data, uint32_t cx, uint32_t cy);
	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

public:
	EmpireVerticalDock(QWidget *parent = nullptr);
	~EmpireVerticalDock();
};
