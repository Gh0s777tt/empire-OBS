#include "EmpireAudioDock.hpp"

#include <obs-frontend-api.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

#include "moc_EmpireAudioDock.cpp"

static void empire_style_mute(QPushButton *b, bool muted)
{
	b->setText(muted ? QStringLiteral("Muted") : QStringLiteral("Mute"));
	b->setStyleSheet(
		muted ? "background:#B20710; color:white; font-weight:600; border-radius:8px; padding:5px 12px;"
		      : "background:#232323; color:#DDD; font-weight:600; border-radius:8px; padding:5px 12px;");
}

EmpireAudioDock::EmpireAudioDock(QWidget *parent) : QFrame(parent)
{
	QVBoxLayout *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);

	QScrollArea *scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QWidget *content = new QWidget();
	rowLayout = new QVBoxLayout(content);
	rowLayout->setContentsMargins(8, 8, 8, 8);
	rowLayout->setSpacing(8);
	scroll->setWidget(content);
	outer->addWidget(scroll);

	Rebuild();
	obs_frontend_add_event_callback(OBSFrontendEvent, this);

	setObjectName(QStringLiteral("empireAudioDock"));
}

EmpireAudioDock::~EmpireAudioDock()
{
	obs_frontend_remove_event_callback(OBSFrontendEvent, this);
	for (obs_fader_t *f : faders)
		obs_fader_destroy(f);
	faders.clear();
}

void EmpireAudioDock::AddSourceRow(obs_source_t *src)
{
	const char *name = obs_source_get_name(src);
	if (!name)
		return;
	const QString qname = QString::fromUtf8(name);

	QFrame *row = new QFrame(this);
	row->setStyleSheet("QFrame { background:#161616; border:1px solid #242424; border-radius:10px; }");
	QHBoxLayout *h = new QHBoxLayout(row);
	h->setContentsMargins(10, 8, 10, 8);
	h->setSpacing(10);

	QLabel *nm = new QLabel(qname, row);
	nm->setMinimumWidth(96);
	nm->setStyleSheet("background:transparent; border:none; color:#FFFFFF; font-weight:600;");
	h->addWidget(nm);

	obs_fader_t *fader = obs_fader_create(OBS_FADER_CUBIC);
	obs_fader_attach_source(fader, src);
	faders.push_back(fader);

	QSlider *sl = new QSlider(Qt::Horizontal, row);
	sl->setRange(0, 100);
	sl->setValue((int)(obs_fader_get_deflection(fader) * 100.0f));
	sl->setStyleSheet("QSlider { background:transparent; border:none; }");
	connect(sl, &QSlider::valueChanged, this,
		[fader](int v) { obs_fader_set_deflection(fader, (float)v / 100.0f); });
	h->addWidget(sl, 1);

	QPushButton *mute = new QPushButton(row);
	empire_style_mute(mute, obs_source_muted(src));
	connect(mute, &QPushButton::clicked, this, [qname, mute]() {
		obs_source_t *s = obs_get_source_by_name(qname.toUtf8().constData());
		if (s) {
			const bool nm2 = !obs_source_muted(s);
			obs_source_set_muted(s, nm2);
			empire_style_mute(mute, nm2);
			obs_source_release(s);
		}
	});
	h->addWidget(mute);

	rowLayout->addWidget(row);
}

void EmpireAudioDock::Rebuild()
{
	if (!rowLayout)
		return;

	/* Remove old rows immediately (before destroying faders the lambdas hold). */
	QLayoutItem *item;
	while ((item = rowLayout->takeAt(0)) != nullptr) {
		if (item->widget())
			delete item->widget();
		delete item;
	}
	for (obs_fader_t *f : faders)
		obs_fader_destroy(f);
	faders.clear();

	obs_enum_sources(
		[](void *p, obs_source_t *src) -> bool {
			EmpireAudioDock *self = static_cast<EmpireAudioDock *>(p);
			if (obs_source_get_output_flags(src) & OBS_SOURCE_AUDIO)
				self->AddSourceRow(src);
			return true;
		},
		this);

	rowLayout->addStretch();
}

void EmpireAudioDock::OBSFrontendEvent(enum obs_frontend_event event, void *ptr)
{
	EmpireAudioDock *dock = static_cast<EmpireAudioDock *>(ptr);

	switch (event) {
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
	case OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED:
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
		dock->Rebuild();
		break;
	default:
		break;
	}
}
