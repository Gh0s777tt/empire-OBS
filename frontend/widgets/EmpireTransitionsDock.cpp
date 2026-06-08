#include "EmpireTransitionsDock.hpp"

#include <obs-frontend-api.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

#include "moc_EmpireTransitionsDock.cpp"

EmpireTransitionsDock::EmpireTransitionsDock(QWidget *parent) : QFrame(parent)
{
	QVBoxLayout *outer = new QVBoxLayout(this);
	outer->setContentsMargins(10, 10, 10, 10);
	outer->setSpacing(8);

	combo = new QComboBox(this);
	combo->setCursor(Qt::PointingHandCursor);
	combo->setStyleSheet("QComboBox { background:#1A1A1A; border:1px solid #2A2A2A; border-radius:8px;"
			     " padding:7px 10px; color:#EDEDED; }"
			     "QComboBox:hover { border-color:#3A3A3A; }"
			     "QComboBox::drop-down { border:none; width:22px; }");
	connect(combo, &QComboBox::currentTextChanged, this, [this](const QString &name) {
		if (syncing || name.isEmpty())
			return;
		struct obs_frontend_source_list ts = {};
		obs_frontend_get_transitions(&ts);
		for (size_t i = 0; i < ts.sources.num; i++) {
			obs_source_t *t = ts.sources.array[i];
			const char *n = obs_source_get_name(t);
			if (n && name == QString::fromUtf8(n)) {
				obs_frontend_set_current_transition(t);
				break;
			}
		}
		obs_frontend_source_list_free(&ts);
	});
	outer->addWidget(combo);

	QHBoxLayout *durRow = new QHBoxLayout();
	durRow->setSpacing(8);

	QLabel *durLabel = new QLabel(QStringLiteral("Czas (ms)"), this);
	durLabel->setStyleSheet("color:#9A9A9A; font-weight:600;");
	durRow->addWidget(durLabel);

	duration = new QSpinBox(this);
	duration->setRange(0, 20000);
	duration->setSingleStep(50);
	duration->setStyleSheet("QSpinBox { background:#1A1A1A; border:1px solid #2A2A2A; border-radius:8px;"
				" padding:6px 8px; color:#EDEDED; }");
	connect(duration, &QSpinBox::valueChanged, this, [this](int v) {
		if (syncing)
			return;
		obs_frontend_set_transition_duration(v);
	});
	durRow->addWidget(duration, 1);
	outer->addLayout(durRow);

	outer->addStretch();

	RebuildList();
	SyncDuration();
	obs_frontend_add_event_callback(OBSFrontendEvent, this);

	setObjectName(QStringLiteral("empireTransitionsDock"));
}

EmpireTransitionsDock::~EmpireTransitionsDock()
{
	obs_frontend_remove_event_callback(OBSFrontendEvent, this);
}

void EmpireTransitionsDock::RebuildList()
{
	syncing = true;
	combo->clear();

	struct obs_frontend_source_list ts = {};
	obs_frontend_get_transitions(&ts);
	for (size_t i = 0; i < ts.sources.num; i++) {
		const char *n = obs_source_get_name(ts.sources.array[i]);
		if (n)
			combo->addItem(QString::fromUtf8(n));
	}
	obs_frontend_source_list_free(&ts);

	obs_source_t *cur = obs_frontend_get_current_transition();
	if (cur) {
		const char *n = obs_source_get_name(cur);
		if (n)
			combo->setCurrentText(QString::fromUtf8(n));
		obs_source_release(cur);
	}
	syncing = false;
}

void EmpireTransitionsDock::SyncCurrent()
{
	obs_source_t *cur = obs_frontend_get_current_transition();
	if (!cur)
		return;
	const char *n = obs_source_get_name(cur);
	syncing = true;
	if (n)
		combo->setCurrentText(QString::fromUtf8(n));
	syncing = false;
	obs_source_release(cur);
}

void EmpireTransitionsDock::SyncDuration()
{
	syncing = true;
	duration->setValue(obs_frontend_get_transition_duration());
	syncing = false;
}

void EmpireTransitionsDock::OBSFrontendEvent(enum obs_frontend_event event, void *ptr)
{
	EmpireTransitionsDock *dock = static_cast<EmpireTransitionsDock *>(ptr);

	switch (event) {
	case OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED:
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
		dock->RebuildList();
		dock->SyncDuration();
		break;
	case OBS_FRONTEND_EVENT_TRANSITION_CHANGED:
		dock->SyncCurrent();
		break;
	case OBS_FRONTEND_EVENT_TRANSITION_DURATION_CHANGED:
		dock->SyncDuration();
		break;
	default:
		break;
	}
}
