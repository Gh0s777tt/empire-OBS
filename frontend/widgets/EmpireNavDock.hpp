#pragma once

/*
 * Empire OBS — right-side navigation rail.
 *
 * The mockup's right page-nav (STREAMY / NAGRANIA / SCENY / USTAWIENIA), done
 * the safe way: each entry is an action/launcher (toggle an Empire dock, open
 * the recordings folder, open Settings) rather than a full central-content
 * page-switcher, so the core OBSBasic layout is never torn apart.
 */

#include <obs.hpp>
#include <obs-frontend-api.h>

#include <QFrame>

class QPushButton;

class EmpireNavDock : public QFrame {
	Q_OBJECT

	QPushButton *MakeNavButton(const QString &label);
	void ToggleDock(const char *id);

public:
	EmpireNavDock(QWidget *parent = nullptr);
};
