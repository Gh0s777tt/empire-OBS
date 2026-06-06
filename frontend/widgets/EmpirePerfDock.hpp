#pragma once

/*
 * Empire OBS — Performance / Stream-Health dock (Tier A #8).
 *
 * Live rolling-line graphs of CPU, FPS, render time, missed (lagged) frames
 * and memory usage. Data is polled from the same libobs APIs used by the
 * built-in stats dock (see widgets/OBSBasicStats.cpp).
 *
 * NOTE: graphs are custom-painted on purpose — the obs-deps Qt6 bundle does
 * not ship the Qt Charts module, so a QPainter-based widget keeps this
 * dependency-free and always builds.
 */

#include <obs.hpp>
#include <util/platform.h>

#include <QFrame>
#include <QWidget>
#include <QColor>
#include <QSize>
#include <QString>
#include <QTimer>

#include <vector>

class QPaintEvent;
class QShowEvent;
class QHideEvent;
class QLabel;

/* Lightweight rolling line-graph (no Qt Charts dependency). */
class EmpireGraph : public QWidget {
	std::vector<double> samples;
	size_t capacity = 120;
	double fixedMax = 0.0; /* 0 => auto-scale to data */
	double peak = 0.0;     /* running maximum since the dock opened */
	QColor lineColor;
	QString caption;
	QString unit;

public:
	EmpireGraph(QString caption, QColor color, QString unit = QString(), double fixedMax = 0.0,
		    QWidget *parent = nullptr);

	void addSample(double value);
	void clear();

protected:
	void paintEvent(QPaintEvent *event) override;
	QSize sizeHint() const override;
};

class EmpirePerfDock : public QFrame {
	Q_OBJECT

	EmpireGraph *cpuGraph = nullptr;
	EmpireGraph *fpsGraph = nullptr;
	EmpireGraph *renderGraph = nullptr;
	EmpireGraph *missedGraph = nullptr;
	EmpireGraph *memGraph = nullptr;
	EmpireGraph *droppedGraph = nullptr;
	EmpireGraph *bitrateGraph = nullptr;

	QLabel *healthBanner = nullptr;

	os_cpu_usage_info_t *cpu_info = nullptr;
	QTimer timer;

	uint32_t first_rendered = 0xFFFFFFFF;
	uint32_t first_lagged = 0xFFFFFFFF;

	/* streaming-output health tracking */
	int first_total = 0;
	int first_dropped = 0;
	uint64_t lastBytes = 0;
	uint64_t lastBytesTime = 0;

	void Update();

public:
	EmpirePerfDock(QWidget *parent = nullptr);
	~EmpirePerfDock();

protected:
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
};
