#pragma once

/*
 * Empire OBS — preview corner badges (experimental).
 *
 * Renders a small status pill (LIVE / REC state · output resolution · current
 * scene) as a Qt-painted RGBA texture and draws it over the main program
 * preview through an obs_display draw callback. The texture is only re-uploaded
 * when the text changes; the draw callback is null-guarded so a missing texture
 * is a harmless no-op. Texture swaps happen under the graphics lock, which is
 * mutually exclusive with the render thread's draw callbacks.
 */

#include <obs.h>

#include <QObject>
#include <QString>
#include <QTimer>

class OBSQTDisplay;

class EmpirePreviewBadges : public QObject {
	Q_OBJECT

public:
	EmpirePreviewBadges(OBSQTDisplay *display, QObject *parent = nullptr);
	~EmpirePreviewBadges() override;

private:
	OBSQTDisplay *qtDisplay = nullptr;
	gs_texture_t *tex = nullptr;
	int texW = 0;
	int texH = 0;
	QString lastKey;
	QTimer timer;

	void Update();
	static void Draw(void *data, uint32_t cx, uint32_t cy);
};
