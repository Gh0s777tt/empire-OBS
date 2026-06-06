#include "EmpirePerfDock.hpp"

#include <obs-frontend-api.h>

#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>

#include <algorithm>

#include "moc_EmpirePerfDock.cpp"

#define EMPIRE_PERF_INTERVAL 1000

/* ============================ EmpireGraph ============================ */

EmpireGraph::EmpireGraph(QString caption, QColor color, QString unit, double fixedMax, QWidget *parent)
	: QWidget(parent),
	  fixedMax(fixedMax),
	  lineColor(color),
	  caption(caption),
	  unit(unit)
{
	samples.reserve(capacity);
	setMinimumHeight(80);
}

QSize EmpireGraph::sizeHint() const
{
	return QSize(240, 90);
}

void EmpireGraph::addSample(double value)
{
	if (value < 0.0)
		value = 0.0;

	samples.push_back(value);
	if (samples.size() > capacity)
		samples.erase(samples.begin());

	update();
}

void EmpireGraph::clear()
{
	samples.clear();
	update();
}

void EmpireGraph::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);

	const QRectF r = QRectF(rect()).adjusted(1, 1, -1, -1);

	/* Card background (Netflix-dark) */
	p.setPen(Qt::NoPen);
	p.setBrush(QColor(0x18, 0x18, 0x18));
	p.drawRoundedRect(r, 8, 8);

	const double latest = samples.empty() ? 0.0 : samples.back();

	/* Scale */
	double maxVal = fixedMax;
	if (maxVal <= 0.0) {
		maxVal = 1.0;
		for (double s : samples)
			maxVal = std::max(maxVal, s);
		maxVal *= 1.15; /* headroom */
	}

	/* Plot area (leave a header strip for the caption) */
	const double top = r.top() + 24.0;
	const double bottom = r.bottom() - 6.0;
	const double left = r.left() + 8.0;
	const double right = r.right() - 8.0;
	const double h = bottom - top;
	const double w = right - left;

	/* Caption + current value */
	p.setPen(QColor(0xB3, 0xB3, 0xB3));
	p.drawText(QRectF(left, r.top() + 4.0, w * 0.6, 18.0), Qt::AlignLeft | Qt::AlignVCenter, caption);

	QString valText = QString::number(latest, 'f', 1);
	if (!unit.isEmpty())
		valText += QStringLiteral(" ") + unit;
	p.setPen(QColor(0xFF, 0xFF, 0xFF));
	p.drawText(QRectF(left + w * 0.4, r.top() + 4.0, w * 0.6, 18.0), Qt::AlignRight | Qt::AlignVCenter, valText);

	if (samples.size() < 2 || w <= 0.0 || h <= 0.0)
		return;

	/* Right-aligned series so the newest sample sits at the right edge */
	const double stepX = w / (double)(capacity - 1);
	const size_t n = samples.size();
	const double startX = right - stepX * (double)(n - 1);

	QPainterPath line;
	for (size_t i = 0; i < n; i++) {
		const double x = startX + stepX * (double)i;
		const double norm = std::min(samples[i] / maxVal, 1.0);
		const double y = bottom - norm * h;
		if (i == 0)
			line.moveTo(x, y);
		else
			line.lineTo(x, y);
	}

	QPainterPath fill = line;
	fill.lineTo(right, bottom);
	fill.lineTo(startX, bottom);
	fill.closeSubpath();

	QColor fillColor = lineColor;
	fillColor.setAlpha(48);
	p.setPen(Qt::NoPen);
	p.setBrush(fillColor);
	p.drawPath(fill);

	QPen pen(lineColor);
	pen.setWidthF(2.0);
	p.setPen(pen);
	p.setBrush(Qt::NoBrush);
	p.drawPath(line);
}

/* ============================ EmpirePerfDock ============================ */

EmpirePerfDock::EmpirePerfDock(QWidget *parent) : QFrame(parent), cpu_info(os_cpu_usage_info_start()), timer(this)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(6, 6, 6, 6);
	layout->setSpacing(6);

	cpuGraph = new EmpireGraph(QStringLiteral("CPU"), QColor(0xE5, 0x09, 0x14), QStringLiteral("%"), 100.0, this);
	fpsGraph = new EmpireGraph(QStringLiteral("FPS"), QColor(0x46, 0xD3, 0x69), QString(), 0.0, this);
	renderGraph =
		new EmpireGraph(QStringLiteral("Render"), QColor(0x3D, 0xBE, 0xF5), QStringLiteral("ms"), 0.0, this);
	missedGraph = new EmpireGraph(QStringLiteral("Missed frames"), QColor(0xEA, 0xBC, 0x48), QStringLiteral("%"),
				     100.0, this);
	memGraph =
		new EmpireGraph(QStringLiteral("Memory"), QColor(0x99, 0x7F, 0xDC), QStringLiteral("MB"), 0.0, this);

	layout->addWidget(cpuGraph);
	layout->addWidget(fpsGraph);
	layout->addWidget(renderGraph);
	layout->addWidget(missedGraph);
	layout->addWidget(memGraph);
	layout->addStretch();

	connect(&timer, &QTimer::timeout, this, &EmpirePerfDock::Update);
	timer.setInterval(EMPIRE_PERF_INTERVAL);

	setObjectName(QStringLiteral("empirePerfDock"));
}

EmpirePerfDock::~EmpirePerfDock()
{
	os_cpu_usage_info_destroy(cpu_info);
}

void EmpirePerfDock::Update()
{
	/* CPU usage (%) */
	cpuGraph->addSample(os_cpu_usage_info_query(cpu_info));

	/* Active render FPS */
	fpsGraph->addSample(obs_get_active_fps());

	/* Average frame render time (ms) */
	renderGraph->addSample((double)obs_get_average_frame_time_ns() / 1000000.0);

	/* Process resident memory (MB) */
	memGraph->addSample((double)os_get_proc_resident_size() / (1024.0 * 1024.0));

	/* Missed (lagged) frames since reset, percentage */
	uint32_t total_rendered = obs_get_total_frames();
	uint32_t total_lagged = obs_get_lagged_frames();

	if (total_rendered < first_rendered || total_lagged < first_lagged) {
		first_rendered = total_rendered;
		first_lagged = total_lagged;
	}

	const uint32_t rendered = total_rendered - first_rendered;
	const uint32_t lagged = total_lagged - first_lagged;
	const double missedPct = rendered ? (double)lagged / (double)rendered * 100.0 : 0.0;
	missedGraph->addSample(missedPct);
}

void EmpirePerfDock::showEvent(QShowEvent *)
{
	timer.start();
	Update();
}

void EmpirePerfDock::hideEvent(QHideEvent *)
{
	timer.stop();
}
