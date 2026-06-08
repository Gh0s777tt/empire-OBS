#include "EmpirePreviewBadges.hpp"
#include "OBSQTDisplay.hpp"

#include <obs-frontend-api.h>

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QRect>

#include "moc_EmpirePreviewBadges.cpp"

EmpirePreviewBadges::EmpirePreviewBadges(OBSQTDisplay *display, QObject *parent) : QObject(parent), qtDisplay(display)
{
	auto addDraw = [this](OBSQTDisplay *win) {
		if (win && win->GetDisplay())
			obs_display_add_draw_callback(win->GetDisplay(), EmpirePreviewBadges::Draw, this);
	};
	if (qtDisplay && qtDisplay->GetDisplay())
		addDraw(qtDisplay);
	else if (qtDisplay)
		connect(qtDisplay, &OBSQTDisplay::DisplayCreated, this, addDraw);

	timer.setInterval(500);
	connect(&timer, &QTimer::timeout, this, &EmpirePreviewBadges::Update);
	timer.start();
	Update();
}

EmpirePreviewBadges::~EmpirePreviewBadges()
{
	timer.stop();
	if (qtDisplay && qtDisplay->GetDisplay())
		obs_display_remove_draw_callback(qtDisplay->GetDisplay(), EmpirePreviewBadges::Draw, this);
	if (tex) {
		obs_enter_graphics();
		gs_texture_destroy(tex);
		obs_leave_graphics();
		tex = nullptr;
	}
}

void EmpirePreviewBadges::Update()
{
	const bool streaming = obs_frontend_streaming_active();
	const bool recording = obs_frontend_recording_active();

	const QString live = streaming ? QStringLiteral("● LIVE")
				       : (recording ? QStringLiteral("● REC") : QStringLiteral("○ OFFLINE"));
	const QColor liveColor = (streaming || recording) ? QColor(0xE5, 0x09, 0x14) : QColor(0xA8, 0xA8, 0xA8);

	struct obs_video_info ovi;
	const QString res = obs_get_video_info(&ovi)
				    ? QStringLiteral("%1×%2").arg(ovi.output_width).arg(ovi.output_height)
				    : QString();

	obs_source_t *scene = obs_frontend_get_current_scene();
	const QString sceneName = scene ? QString::fromUtf8(obs_source_get_name(scene)) : QString();
	obs_source_release(scene);

	const QString key = live + QStringLiteral("|") + res + QStringLiteral("|") + sceneName;
	if (key == lastKey && tex)
		return;
	lastKey = key;

	QString rightText;
	if (!res.isEmpty())
		rightText = res;
	if (!sceneName.isEmpty())
		rightText += (rightText.isEmpty() ? QString() : QStringLiteral("    ")) + sceneName;

	QFont font;
	font.setPointSize(10);
	font.setBold(true);
	const QFontMetrics fm(font);

	const int pad = 12;
	const int gap = 16;
	const int H = 30;
	const int liveW = fm.horizontalAdvance(live);
	const int rightW = rightText.isEmpty() ? 0 : fm.horizontalAdvance(rightText);
	const int W = pad + liveW + (rightText.isEmpty() ? 0 : gap + rightW) + pad;

	QImage img(W, H, QImage::Format_RGBA8888);
	img.fill(Qt::transparent);
	{
		QPainter p(&img);
		p.setRenderHint(QPainter::Antialiasing, true);
		p.setRenderHint(QPainter::TextAntialiasing, true);
		p.setPen(Qt::NoPen);
		p.setBrush(QColor(0, 0, 0, 170));
		p.drawRoundedRect(0, 0, W, H, 8, 8);
		p.setFont(font);
		p.setPen(liveColor);
		p.drawText(QRect(pad, 0, liveW, H), Qt::AlignVCenter | Qt::AlignLeft, live);
		if (!rightText.isEmpty()) {
			p.setPen(QColor(0xEC, 0xEC, 0xEC));
			p.drawText(QRect(pad + liveW + gap, 0, rightW, H), Qt::AlignVCenter | Qt::AlignLeft, rightText);
		}
	}

	const uint8_t *bits = img.constBits();
	obs_enter_graphics();
	if (tex) {
		gs_texture_destroy(tex);
		tex = nullptr;
	}
	tex = gs_texture_create((uint32_t)W, (uint32_t)H, GS_RGBA, 1, &bits, 0);
	texW = W;
	texH = H;
	obs_leave_graphics();
}

void EmpirePreviewBadges::Draw(void *data, uint32_t cx, uint32_t cy)
{
	EmpirePreviewBadges *self = static_cast<EmpirePreviewBadges *>(data);
	gs_texture_t *tex = self->tex;
	const int W = self->texW;
	const int H = self->texH;
	if (!tex || W <= 0 || H <= 0)
		return;

	const float pad = 12.0f;

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();

	gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f);
	gs_set_viewport(0, 0, (int)cx, (int)cy);

	gs_enable_blending(true);
	gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);

	gs_matrix_identity();
	gs_matrix_translate3f(pad, pad, 0.0f);

	gs_effect_t *eff = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_eparam_t *image = gs_effect_get_param_by_name(eff, "image");
	gs_effect_set_texture(image, tex);
	while (gs_effect_loop(eff, "Draw"))
		gs_draw_sprite(tex, 0, (uint32_t)W, (uint32_t)H);

	gs_matrix_pop();
	gs_projection_pop();
	gs_viewport_pop();
}
