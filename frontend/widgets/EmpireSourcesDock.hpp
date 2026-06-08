#pragma once

/*
 * Empire OBS — Sources panel (UI rebuild, phase 4).
 *
 * The current scene's sources as cards with a one-click show/hide toggle.
 * Visible sources keep a red accent bar; hidden ones dim out.
 */

#include <obs.hpp>
#include <obs-frontend-api.h>

#include <QFrame>

class QVBoxLayout;

class EmpireSourcesDock : public QFrame {
	Q_OBJECT

	QVBoxLayout *rowLayout = nullptr;

	void Rebuild();
	void AddItemRow(obs_sceneitem_t *item);
	void ShowAddMenu(const QPoint &globalPos);
	void AddSourceOfType(const char *id);
	void SourceCardMenu(const QString &name, const QPoint &globalPos);

	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

public:
	EmpireSourcesDock(QWidget *parent = nullptr);
	~EmpireSourcesDock();
};
