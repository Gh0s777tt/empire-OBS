#include "EmpireAudioDock.hpp"

#include <obs-frontend-api.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
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

static QString empire_fmt_db(float db)
{
	if (db <= -96.0f)
		return QStringLiteral("-∞ dB");
	return QStringLiteral("%1 dB").arg(QString::number(db, 'f', 1));
}

EmpireLevelBar::EmpireLevelBar(QWidget *parent) : QWidget(parent)
{
	setFixedHeight(6);
	setAttribute(Qt::WA_TransparentForMouseEvents);
}

void EmpireLevelBar::refresh()
{
	const float m = magnitude.load(std::memory_order_relaxed);
	if (m > displayed)
		displayed = m;
	else
		displayed *= 0.82f;
	if (displayed < 0.0015f)
		displayed = 0.0f;
	update();
}

void EmpireLevelBar::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	const QRect r = rect();
	p.fillRect(r, QColor(0x0E, 0x0E, 0x0E));
	const int w = static_cast<int>(r.width() * displayed);
	if (w > 0) {
		QLinearGradient grad(0, 0, r.width(), 0);
		grad.setColorAt(0.0, QColor(0x2E, 0xCC, 0x71));
		grad.setColorAt(0.70, QColor(0x2E, 0xCC, 0x71));
		grad.setColorAt(0.86, QColor(0xE5, 0xA5, 0x0A));
		grad.setColorAt(1.0, QColor(0xE5, 0x09, 0x14));
		p.fillRect(0, 0, w, r.height(), QBrush(grad));
	}
}

void EmpireAudioDock::VolmeterCallback(void *param, const float magnitude[MAX_AUDIO_CHANNELS],
				       const float peak[MAX_AUDIO_CHANNELS], const float input_peak[MAX_AUDIO_CHANNELS])
{
	(void)magnitude;
	(void)input_peak;
	float db = -100.0f;
	for (int i = 0; i < 2 && i < MAX_AUDIO_CHANNELS; i++) {
		if (peak[i] > db)
			db = peak[i];
	}
	float norm = (db + 60.0f) / 60.0f;
	if (norm < 0.0f)
		norm = 0.0f;
	if (norm > 1.0f)
		norm = 1.0f;
	static_cast<EmpireLevelBar *>(param)->setMagnitude(norm);
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

	meterTimer.setInterval(40);
	connect(&meterTimer, &QTimer::timeout, this, [this]() {
		for (EmpireLevelBar *m : meters)
			m->refresh();
	});
	meterTimer.start();

	Rebuild();
	obs_frontend_add_event_callback(OBSFrontendEvent, this);

	setObjectName(QStringLiteral("empireAudioDock"));
}

EmpireAudioDock::~EmpireAudioDock()
{
	obs_frontend_remove_event_callback(OBSFrontendEvent, this);
	meterTimer.stop();
	for (size_t i = 0; i < volmeters.size(); i++) {
		obs_volmeter_remove_callback(volmeters[i], VolmeterCallback, meters[i]);
		obs_volmeter_destroy(volmeters[i]);
	}
	volmeters.clear();
	meters.clear();
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
	row->setStyleSheet("QFrame { background:#1A1A1A; border:1px solid #2A2A2A; border-radius:10px; }");
	QVBoxLayout *v = new QVBoxLayout(row);
	v->setContentsMargins(10, 8, 10, 8);
	v->setSpacing(6);

	QHBoxLayout *h = new QHBoxLayout();
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

	QLabel *db = new QLabel(empire_fmt_db(obs_fader_get_db(fader)), row);
	db->setMinimumWidth(58);
	db->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	db->setStyleSheet("background:transparent; border:none; color:#9A9A9A; font-weight:600;");

	connect(sl, &QSlider::valueChanged, this, [fader, db](int v) {
		obs_fader_set_deflection(fader, (float)v / 100.0f);
		db->setText(empire_fmt_db(obs_fader_get_db(fader)));
	});
	h->addWidget(sl, 1);
	h->addWidget(db);

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

	v->addLayout(h);

	EmpireLevelBar *meter = new EmpireLevelBar(row);
	v->addWidget(meter);

	obs_volmeter_t *vm = obs_volmeter_create(OBS_FADER_LOG);
	obs_volmeter_attach_source(vm, src);
	obs_volmeter_add_callback(vm, VolmeterCallback, meter);
	volmeters.push_back(vm);
	meters.push_back(meter);

	rowLayout->addWidget(row);
}

void EmpireAudioDock::Rebuild()
{
	if (!rowLayout)
		return;

	/* Tear down meters first (the volmeter callback writes into the bar widget),
	 * then remove the rows, then destroy the faders the slider lambdas hold. */
	meterTimer.stop();
	for (size_t i = 0; i < volmeters.size(); i++) {
		obs_volmeter_remove_callback(volmeters[i], VolmeterCallback, meters[i]);
		obs_volmeter_destroy(volmeters[i]);
	}
	volmeters.clear();
	meters.clear();

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
	meterTimer.start();
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
