#include "EmpireSourcesDock.hpp"

#include <obs-frontend-api.h>

#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

#include "moc_EmpireSourcesDock.cpp"

static const char *kSourceStyle =
	"QPushButton { background:#1A1A1A; border:1px solid #2A2A2A; border-left:3px solid #2A2A2A;"
	" border-radius:8px; padding:10px 14px; color:#777777; text-align:left; font-weight:500; }"
	"QPushButton:hover { background:#232323; }"
	"QPushButton:checked { color:#FFFFFF; border-left:3px solid #E50914; }";

EmpireSourcesDock::EmpireSourcesDock(QWidget *parent) : QFrame(parent)
{
	QVBoxLayout *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);

	QScrollArea *scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QWidget *content = new QWidget();
	rowLayout = new QVBoxLayout(content);
	rowLayout->setContentsMargins(6, 6, 6, 6);
	rowLayout->setSpacing(6);
	scroll->setWidget(content);
	outer->addWidget(scroll);

	Rebuild();
	obs_frontend_add_event_callback(OBSFrontendEvent, this);

	setObjectName(QStringLiteral("empireSourcesDock"));
}

EmpireSourcesDock::~EmpireSourcesDock()
{
	obs_frontend_remove_event_callback(OBSFrontendEvent, this);
}

void EmpireSourcesDock::AddItemRow(obs_sceneitem_t *item)
{
	obs_source_t *src = obs_sceneitem_get_source(item);
	const char *name = src ? obs_source_get_name(src) : nullptr;
	if (!name)
		return;
	const QString qname = QString::fromUtf8(name);

	QPushButton *card = new QPushButton(qname, this);
	card->setCheckable(true);
	card->setChecked(obs_sceneitem_visible(item));
	card->setCursor(Qt::PointingHandCursor);
	card->setStyleSheet(kSourceStyle);

	/* Re-look the item up by name on click so a removed item can never dangle. */
	connect(card, &QPushButton::clicked, this, [qname, card]() {
		obs_source_t *sceneSrc = obs_frontend_get_current_scene();
		obs_scene_t *scene = obs_scene_from_source(sceneSrc);
		obs_sceneitem_t *it = scene ? obs_scene_find_source(scene, qname.toUtf8().constData()) : nullptr;
		if (it) {
			const bool vis = !obs_sceneitem_visible(it);
			obs_sceneitem_set_visible(it, vis);
			card->setChecked(vis);
		} else {
			card->setChecked(false);
		}
		obs_source_release(sceneSrc);
	});

	rowLayout->addWidget(card);
}

void EmpireSourcesDock::Rebuild()
{
	if (!rowLayout)
		return;

	QLayoutItem *item;
	while ((item = rowLayout->takeAt(0)) != nullptr) {
		if (item->widget())
			delete item->widget();
		delete item;
	}

	obs_source_t *sceneSrc = obs_frontend_get_current_scene();
	obs_scene_t *scene = obs_scene_from_source(sceneSrc);
	if (scene) {
		obs_scene_enum_items(
			scene,
			[](obs_scene_t *, obs_sceneitem_t *it, void *p) -> bool {
				static_cast<EmpireSourcesDock *>(p)->AddItemRow(it);
				return true;
			},
			this);
	}
	obs_source_release(sceneSrc);

	rowLayout->addStretch();
}

void EmpireSourcesDock::OBSFrontendEvent(enum obs_frontend_event event, void *ptr)
{
	EmpireSourcesDock *dock = static_cast<EmpireSourcesDock *>(ptr);

	switch (event) {
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
		dock->Rebuild();
		break;
	default:
		break;
	}
}
