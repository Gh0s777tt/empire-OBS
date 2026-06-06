#include "EmpireVerticalDock.hpp"
#include "OBSQTDisplay.hpp"

#include <utility/display-helpers.hpp>

#include <obs-frontend-api.h>
#include <graphics/graphics.h>

#include <QVBoxLayout>

#include "moc_EmpireVerticalDock.cpp"

#define VERTICAL_W 1080
#define VERTICAL_H 1920

EmpireVerticalDock::EmpireVerticalDock(QWidget *parent) : QFrame(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	/* Private 9:16 canvas, inheriting the main video timing/format. */
	obs_video_info ovi = {};
	obs_get_video_info(&ovi);
	ovi.base_width = VERTICAL_W;
	ovi.base_height = VERTICAL_H;
	ovi.output_width = VERTICAL_W;
	ovi.output_height = VERTICAL_H;
	canvas = obs_canvas_create_private("Empire Vertical", &ovi, ACTIVATE);

	display = new OBSQTDisplay(this);
	layout->addWidget(display);

	auto addDraw = [this](OBSQTDisplay *d) {
		obs_display_add_draw_callback(d->GetDisplay(), EmpireVerticalDock::RenderVertical, this);
	};
	connect(display, &OBSQTDisplay::DisplayCreated, this, addDraw);

	SyncToCurrentScene();
	obs_frontend_add_event_callback(OBSFrontendEvent, this);

	setObjectName(QStringLiteral("empireVerticalDock"));
}

EmpireVerticalDock::~EmpireVerticalDock()
{
	obs_frontend_remove_event_callback(OBSFrontendEvent, this);

	/* Remove the draw callback BEFORE tearing down the canvas so the
	 * graphics thread can never touch a freed canvas. */
	if (display && display->GetDisplay())
		obs_display_remove_draw_callback(display->GetDisplay(), EmpireVerticalDock::RenderVertical, this);

	if (canvas) {
		obs_canvas_set_channel(canvas, 0, nullptr);
		obs_canvas_remove(canvas);
		obs_canvas_release(canvas);
		canvas = nullptr;
	}
}

void EmpireVerticalDock::SyncToCurrentScene()
{
	if (!canvas)
		return;

	/* Mirror the current program scene into the vertical canvas. */
	obs_source_t *scene = obs_frontend_get_current_scene();
	obs_canvas_set_channel(canvas, 0, scene);
	obs_source_release(scene);
}

void EmpireVerticalDock::OBSFrontendEvent(enum obs_frontend_event event, void *ptr)
{
	EmpireVerticalDock *dock = static_cast<EmpireVerticalDock *>(ptr);

	switch (event) {
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		dock->SyncToCurrentScene();
		break;
	default:
		break;
	}
}

void EmpireVerticalDock::RenderVertical(void *data, uint32_t, uint32_t)
{
	EmpireVerticalDock *self = static_cast<EmpireVerticalDock *>(data);

	obs_canvas_t *canvas = self->canvas;
	if (!canvas)
		return;

	obs_video_info ovi;
	if (!obs_canvas_get_video_info(canvas, &ovi))
		return;

	uint32_t dw = 0, dh = 0;
	obs_display_size(self->display->GetDisplay(), &dw, &dh);

	int x = 0, y = 0;
	float scale = 1.0f;
	GetScaleAndCenterPos((int)ovi.base_width, (int)ovi.base_height, (int)dw, (int)dh, x, y, scale);

	const int cx = int(scale * float(ovi.base_width));
	const int cy = int(scale * float(ovi.base_height));

	gs_viewport_push();
	gs_projection_push();

	gs_ortho(0.0f, float(ovi.base_width), 0.0f, float(ovi.base_height), -100.0f, 100.0f);
	gs_set_viewport(x, y, cx, cy);

	obs_render_canvas_texture(canvas);

	gs_projection_pop();
	gs_viewport_pop();
}
