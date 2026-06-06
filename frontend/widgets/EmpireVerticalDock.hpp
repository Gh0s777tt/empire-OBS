#pragma once

/*
 * Empire OBS — Vertical 9:16 dock.
 *
 * Hosts a private 1080x1920 libobs canvas rendered into an OBSQTDisplay. The
 * canvas owns a vertical scene that, in MIRROR mode, mirrors the program scene
 * (Fill/Fit). In CUSTOM mode auto-sync is frozen and the scene becomes a
 * composable 9:16 layout: drag to move, wheel to scale the selected item.
 * The vertical canvas can be recorded and streamed (RTMP) independently of the
 * main 16:9 output.
 */

#include "OBSQTDisplay.hpp"

#include <obs.hpp>
#include <obs-frontend-api.h>

#include <QContextMenuEvent>
#include <QFrame>
#include <QMouseEvent>
#include <QWheelEvent>

class QPushButton;
class QLineEdit;

/* OBSQTDisplay that forwards mouse events so the dock can edit the scene. */
class EmpireVerticalDisplay : public OBSQTDisplay {
	Q_OBJECT

public:
	EmpireVerticalDisplay(QWidget *parent = nullptr) : OBSQTDisplay(parent) {}

signals:
	void mousePressedAt(QPointF pos);
	void mouseDraggedAt(QPointF pos);
	void mouseReleasedHere();
	void wheelScaled(int delta);
	void contextMenuRequested(QPoint globalPos);

protected:
	void mousePressEvent(QMouseEvent *e) override
	{
		if (e->button() == Qt::LeftButton)
			emit mousePressedAt(e->position());
	}
	void mouseMoveEvent(QMouseEvent *e) override
	{
		if (e->buttons() & Qt::LeftButton)
			emit mouseDraggedAt(e->position());
	}
	void mouseReleaseEvent(QMouseEvent *) override { emit mouseReleasedHere(); }
	void wheelEvent(QWheelEvent *e) override { emit wheelScaled(e->angleDelta().y()); }
	void contextMenuEvent(QContextMenuEvent *e) override { emit contextMenuRequested(e->globalPos()); }
};

class EmpireVerticalDock : public QFrame {
	Q_OBJECT

	EmpireVerticalDisplay *display = nullptr;
	QPushButton *modeButton = nullptr;
	QPushButton *customButton = nullptr;
	QPushButton *recordButton = nullptr;
	QPushButton *streamButton = nullptr;
	QLineEdit *urlEdit = nullptr;
	QLineEdit *keyEdit = nullptr;

	obs_canvas_t *canvas = nullptr;
	obs_scene_t *vScene = nullptr;         /* borrowed — owned by the canvas */
	obs_sceneitem_t *mirrorItem = nullptr; /* the program-mirror item in vScene */
	bool fillMode = true;                  /* true = Fill/crop, false = Fit/letterbox */
	bool customMode = false;               /* true = freeze sync + edit the scene */

	/* editing state */
	obs_sceneitem_t *selectedItem = nullptr;
	bool dragging = false;
	float previewX = 0.0f, previewY = 0.0f, previewScale = 1.0f; /* last render transform (device px) */
	float dragStartCx = 0.0f, dragStartCy = 0.0f;                /* mouse-down position in canvas space */
	float itemStartX = 0.0f, itemStartY = 0.0f;                  /* item position at mouse-down */

	int addCounter = 0; /* for unique added-source names */

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
	void ToggleCustomMode();
	void UpdateCustomButton();
	void ToggleRecording();
	void StartRecording();
	void UpdateRecordButton();
	void ToggleStreaming();
	void StartStreaming();
	void UpdateStreamButton();
	void LoadConfig();
	void SaveConfig();

	/* editing helpers */
	bool WidgetToCanvas(const QPointF &pos, float &cx, float &cy) const;
	obs_sceneitem_t *HitTest(float cx, float cy) const;
	void OnMousePress(const QPointF &pos);
	void OnMouseDrag(const QPointF &pos);
	void OnWheel(int delta);
	void ShowContextMenu(QPoint globalPos);
	void AddSource(const char *id);
	void RemoveSelected();

	static void RenderVertical(void *data, uint32_t cx, uint32_t cy);
	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

public:
	EmpireVerticalDock(QWidget *parent = nullptr);
	~EmpireVerticalDock();
};
