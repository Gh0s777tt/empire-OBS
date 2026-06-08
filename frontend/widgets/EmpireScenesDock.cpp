#include "EmpireScenesDock.hpp"

#include <obs-frontend-api.h>

#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMetaObject>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

#include "moc_EmpireScenesDock.cpp"

static const char *kCardStyle = "QPushButton {"
				"  background:#1A1A1A; border:1px solid #2A2A2A; border-radius:10px;"
				"  padding:11px 14px; color:#DDDDDD; text-align:left; font-weight:500;"
				"}"
				"QPushButton:hover { background:#232323; border-color:#3C3C3C; }"
				"QPushButton:checked {"
				"  background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #E50914, stop:1 #B20710);"
				"  border-color:#E50914; color:#FFFFFF; font-weight:600;"
				"}";

EmpireScenesDock::EmpireScenesDock(QWidget *parent) : QFrame(parent)
{
	QVBoxLayout *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);
	outer->setSpacing(0);

	QPushButton *addBtn = new QPushButton(QStringLiteral("+  Nowa scena"), this);
	addBtn->setCursor(Qt::PointingHandCursor);
	addBtn->setMinimumHeight(34);
	addBtn->setStyleSheet("QPushButton { background:#1A1A1A; border:none; border-bottom:1px solid #2A2A2A;"
			      " color:#B3B3B3; font-weight:600; }"
			      "QPushButton:hover { background:#232323; color:#FFFFFF; }");
	connect(addBtn, &QPushButton::clicked, this, []() {
		QWidget *mw = static_cast<QWidget *>(obs_frontend_get_main_window());
		if (mw)
			QMetaObject::invokeMethod(mw, "on_actionAddScene_triggered", Qt::QueuedConnection);
	});
	outer->addWidget(addBtn);

	QScrollArea *scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QWidget *content = new QWidget();
	cardLayout = new QVBoxLayout(content);
	cardLayout->setContentsMargins(6, 6, 6, 6);
	cardLayout->setSpacing(6);
	scroll->setWidget(content);
	outer->addWidget(scroll);

	RebuildScenes();
	obs_frontend_add_event_callback(OBSFrontendEvent, this);

	setObjectName(QStringLiteral("empireScenesDock"));
}

EmpireScenesDock::~EmpireScenesDock()
{
	obs_frontend_remove_event_callback(OBSFrontendEvent, this);
}

void EmpireScenesDock::RebuildScenes()
{
	if (!cardLayout)
		return;

	/* Clear existing cards. */
	QLayoutItem *item;
	while ((item = cardLayout->takeAt(0)) != nullptr) {
		if (item->widget())
			item->widget()->deleteLater();
		delete item;
	}

	struct obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);
	for (size_t i = 0; i < scenes.sources.num; i++) {
		const char *name = obs_source_get_name(scenes.sources.array[i]);
		if (!name)
			continue;
		const QString qname = QString::fromUtf8(name);

		QPushButton *card = new QPushButton(qname, this);
		card->setCheckable(true);
		card->setMinimumHeight(44);
		card->setCursor(Qt::PointingHandCursor);
		card->setProperty("sceneName", qname);
		card->setStyleSheet(kCardStyle);
		connect(card, &QPushButton::clicked, this, [qname]() {
			obs_source_t *s = obs_get_source_by_name(qname.toUtf8().constData());
			if (s) {
				obs_frontend_set_current_scene(s);
				obs_source_release(s);
			}
		});
		card->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(card, &QWidget::customContextMenuRequested, this,
			[this, qname, card](const QPoint &pos) { SceneCardMenu(qname, card->mapToGlobal(pos)); });
		cardLayout->addWidget(card);
	}
	obs_frontend_source_list_free(&scenes);

	cardLayout->addStretch();
	UpdateActive();
}

void EmpireScenesDock::UpdateActive()
{
	if (!cardLayout)
		return;

	obs_source_t *cur = obs_frontend_get_current_scene();
	const QString curName = cur ? QString::fromUtf8(obs_source_get_name(cur)) : QString();
	obs_source_release(cur);

	for (int i = 0; i < cardLayout->count(); i++) {
		QWidget *w = cardLayout->itemAt(i)->widget();
		QPushButton *btn = qobject_cast<QPushButton *>(w);
		if (btn)
			btn->setChecked(btn->property("sceneName").toString() == curName);
	}
}

void EmpireScenesDock::SceneCardMenu(const QString &sceneName, const QPoint &globalPos)
{
	QMenu menu(this);
	QAction *renameAct = menu.addAction(QStringLiteral("Zmień nazwę"));
	QAction *dupAct = menu.addAction(QStringLiteral("Duplikuj"));
	menu.addSeparator();
	QAction *removeAct = menu.addAction(QStringLiteral("Usuń scenę"));

	QAction *chosen = menu.exec(globalPos);
	if (!chosen)
		return;

	if (chosen == renameAct) {
		bool ok = false;
		QString nn = QInputDialog::getText(this, QStringLiteral("Zmień nazwę sceny"),
						   QStringLiteral("Nowa nazwa:"), QLineEdit::Normal, sceneName, &ok)
				     .trimmed();
		if (!ok || nn.isEmpty() || nn == sceneName)
			return;
		obs_source_t *exist = obs_get_source_by_name(nn.toUtf8().constData());
		if (exist) {
			obs_source_release(exist);
			QMessageBox::warning(this, QStringLiteral("Zmień nazwę"),
					     QStringLiteral("Źródło o tej nazwie już istnieje."));
			return;
		}
		obs_source_t *s = obs_get_source_by_name(sceneName.toUtf8().constData());
		if (s) {
			obs_source_set_name(s, nn.toUtf8().constData());
			obs_source_release(s);
		}
	} else if (chosen == dupAct) {
		obs_source_t *s = obs_get_source_by_name(sceneName.toUtf8().constData());
		obs_scene_t *scene = s ? obs_scene_from_source(s) : nullptr;
		if (scene) {
			QString nn = sceneName + QStringLiteral(" (kopia)");
			for (int n = 2;; n++) {
				obs_source_t *e = obs_get_source_by_name(nn.toUtf8().constData());
				if (!e)
					break;
				obs_source_release(e);
				nn = sceneName + QStringLiteral(" (kopia %1)").arg(n);
			}
			obs_scene_t *dup = obs_scene_duplicate(scene, nn.toUtf8().constData(), OBS_SCENE_DUP_REFS);
			if (dup)
				obs_scene_release(dup);
		}
		if (s)
			obs_source_release(s);
	} else if (chosen == removeAct) {
		struct obs_frontend_source_list scenes = {};
		obs_frontend_get_scenes(&scenes);
		const size_t count = scenes.sources.num;
		obs_frontend_source_list_free(&scenes);
		if (count <= 1) {
			QMessageBox::warning(this, QStringLiteral("Usuń scenę"),
					     QStringLiteral("Nie można usunąć ostatniej sceny."));
			return;
		}
		if (QMessageBox::question(this, QStringLiteral("Usuń scenę"),
					  QStringLiteral("Usunąć scenę \"%1\"?").arg(sceneName)) != QMessageBox::Yes)
			return;
		obs_source_t *s = obs_get_source_by_name(sceneName.toUtf8().constData());
		if (s) {
			obs_source_remove(s);
			obs_source_release(s);
		}
	}
}

void EmpireScenesDock::OBSFrontendEvent(enum obs_frontend_event event, void *ptr)
{
	EmpireScenesDock *dock = static_cast<EmpireScenesDock *>(ptr);

	switch (event) {
	case OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED:
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
		dock->RebuildScenes();
		break;
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
	case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
		dock->UpdateActive();
		break;
	default:
		break;
	}
}
