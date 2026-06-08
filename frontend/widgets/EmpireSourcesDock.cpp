#include "EmpireSourcesDock.hpp"

#include <obs-frontend-api.h>

#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

#include "moc_EmpireSourcesDock.cpp"

static const char *kSourceStyle =
	"QPushButton { background:#1A1A1A; border:1px solid #2A2A2A; border-left:3px solid #2A2A2A;"
	" border-radius:10px; padding:10px 14px; color:#777777; text-align:left; font-weight:500; }"
	"QPushButton:hover { background:#232323; }"
	"QPushButton:checked { color:#FFFFFF; border-left:3px solid #E50914; }";

/* Visibility marker: a filled eye-dot when shown, an empty ring when hidden. */
static QString empire_src_label(bool visible, const QString &name)
{
	return (visible ? QStringLiteral("◉  ") : QStringLiteral("○  ")) + name;
}

EmpireSourcesDock::EmpireSourcesDock(QWidget *parent) : QFrame(parent)
{
	QVBoxLayout *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);
	outer->setSpacing(0);

	QPushButton *addBtn = new QPushButton(QStringLiteral("+  Dodaj źródło"), this);
	addBtn->setCursor(Qt::PointingHandCursor);
	addBtn->setMinimumHeight(34);
	addBtn->setStyleSheet("QPushButton { background:#1A1A1A; border:none; border-bottom:1px solid #2A2A2A;"
			      " color:#B3B3B3; font-weight:600; }"
			      "QPushButton:hover { background:#232323; color:#FFFFFF; }");
	connect(addBtn, &QPushButton::clicked, this,
		[this, addBtn]() { ShowAddMenu(addBtn->mapToGlobal(QPoint(0, addBtn->height()))); });
	outer->addWidget(addBtn);

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
	const bool visible = obs_sceneitem_visible(item);

	QPushButton *card = new QPushButton(empire_src_label(visible, qname), this);
	card->setCheckable(true);
	card->setChecked(visible);
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
			card->setText(empire_src_label(vis, qname));
		} else {
			card->setChecked(false);
			card->setText(empire_src_label(false, qname));
		}
		obs_source_release(sceneSrc);
	});

	card->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(card, &QWidget::customContextMenuRequested, this,
		[this, qname, card](const QPoint &pos) { SourceCardMenu(qname, card->mapToGlobal(pos)); });

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

void EmpireSourcesDock::ShowAddMenu(const QPoint &globalPos)
{
	QMenu menu(this);
	static const char *ids[] = {"dshow_input",     "wasapi_input_capture", "wasapi_output_capture",
				    "monitor_capture", "window_capture",       "game_capture",
				    "image_source",    "color_source",         "text_gdiplus",
				    "ffmpeg_source",   "browser_source"};
	for (const char *id : ids) {
		const char *dn = obs_source_get_display_name(id);
		if (!dn)
			continue;
		const QString sid = QString::fromUtf8(id);
		menu.addAction(QString::fromUtf8(dn), this,
			       [this, sid]() { AddSourceOfType(sid.toUtf8().constData()); });
	}
	if (!menu.isEmpty())
		menu.exec(globalPos);
}

void EmpireSourcesDock::AddSourceOfType(const char *id)
{
	const char *dn = obs_source_get_display_name(id);
	const QString base = dn ? QString::fromUtf8(dn) : QString::fromUtf8(id);
	QString defName = base;
	for (int n = 2;; n++) {
		obs_source_t *e = obs_get_source_by_name(defName.toUtf8().constData());
		if (!e)
			break;
		obs_source_release(e);
		defName = QStringLiteral("%1 %2").arg(base).arg(n);
	}

	bool ok = false;
	const QString name = QInputDialog::getText(this, QStringLiteral("Dodaj źródło"),
						   QStringLiteral("Nazwa źródła:"), QLineEdit::Normal, defName, &ok)
				     .trimmed();
	if (!ok || name.isEmpty())
		return;

	obs_source_t *existing = obs_get_source_by_name(name.toUtf8().constData());
	if (existing) {
		obs_source_release(existing);
		QMessageBox::warning(this, QStringLiteral("Dodaj źródło"),
				     QStringLiteral("Źródło o tej nazwie już istnieje."));
		return;
	}

	obs_source_t *sceneSrc = obs_frontend_get_current_scene();
	obs_scene_t *scene = obs_scene_from_source(sceneSrc);
	if (scene) {
		obs_source_t *source = obs_source_create(id, name.toUtf8().constData(), nullptr, nullptr);
		if (source) {
			obs_scene_add(scene, source);
			obs_frontend_open_source_properties(source);
			obs_source_release(source);
		}
	}
	obs_source_release(sceneSrc);
}

void EmpireSourcesDock::SourceCardMenu(const QString &name, const QPoint &globalPos)
{
	QMenu menu(this);
	QAction *propsAct = menu.addAction(QStringLiteral("Właściwości…"));
	QAction *filtersAct = menu.addAction(QStringLiteral("Filtry…"));
	menu.addSeparator();
	QAction *removeAct = menu.addAction(QStringLiteral("Usuń źródło"));

	QAction *chosen = menu.exec(globalPos);
	if (!chosen)
		return;

	obs_source_t *sceneSrc = obs_frontend_get_current_scene();
	obs_scene_t *scene = obs_scene_from_source(sceneSrc);
	obs_sceneitem_t *it = scene ? obs_scene_find_source(scene, name.toUtf8().constData()) : nullptr;
	obs_source_t *src = it ? obs_sceneitem_get_source(it) : nullptr;

	if (src) {
		if (chosen == propsAct) {
			obs_frontend_open_source_properties(src);
		} else if (chosen == filtersAct) {
			obs_frontend_open_source_filters(src);
		} else if (chosen == removeAct) {
			if (QMessageBox::question(this, QStringLiteral("Usuń źródło"),
						  QStringLiteral("Usunąć \"%1\" z bieżącej sceny?").arg(name)) ==
			    QMessageBox::Yes)
				obs_sceneitem_remove(it);
		}
	}
	obs_source_release(sceneSrc);
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
