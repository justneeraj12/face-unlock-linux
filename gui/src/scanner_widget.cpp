#include "scanner_widget.h"

#include <QApplication>
#include <QColor>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QImage>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QShowEvent>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

namespace face_unlock_gui {
namespace {

constexpr int kAnimationIntervalMs = 50;
constexpr double kPi = 3.14159265358979323846;

double clamp01(double value) {
  return std::clamp(value, 0.0, 1.0);
}

double smoothstep(double value) {
  const double x = clamp01(value);
  return x * x * (3.0 - 2.0 * x);
}

double gaussian(double x, double y, double cx, double cy, double spread) {
  const double dx = x - cx;
  const double dy = y - cy;
  return std::exp(-(dx * dx + dy * dy) / spread);
}

QColor mixed(const QColor& first, const QColor& second, double amount) {
  const double t = clamp01(amount);
  return QColor(
    static_cast<int>(first.red() + (second.red() - first.red()) * t),
    static_cast<int>(first.green() + (second.green() - first.green()) * t),
    static_cast<int>(first.blue() + (second.blue() - first.blue()) * t),
    static_cast<int>(first.alpha() + (second.alpha() - first.alpha()) * t)
  );
}

}  // namespace

ScannerWidget::ScannerWidget(QWidget* parent) : QWidget(parent) {
  setAccessibleName(QStringLiteral("Face profile scanner visualization"));
  setMinimumHeight(270);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  buildPointCloud();
  timer_.setInterval(kAnimationIntervalMs);
  timer_.setTimerType(Qt::CoarseTimer);
  connect(&timer_, &QTimer::timeout, this, [this]() {
    if (!fixedPhase_) phase_ = std::fmod(phase_ + 0.025, 1.0);
    update();
  });
}

void ScannerWidget::setScannerState(State state) {
  if (state_ == state) return;
  state_ = state;
  fixedPhase_ = false;
  updateTimer();
  update();
}

void ScannerWidget::setProgress(int progress) {
  const int bounded = std::clamp(progress, 0, 100);
  if (progress_ == bounded) return;
  progress_ = bounded;
  update();
}

void ScannerWidget::setGuidance(const QString& guidance) {
  const QString normalized = guidance.trimmed().toUpper();
  if (guidance_ == normalized) return;
  guidance_ = normalized.isEmpty() ? QStringLiteral("READY") : normalized;
  setAccessibleDescription(stateLabel() + QStringLiteral(". ") + guidance_);
  update();
}

void ScannerWidget::setAnimationPhaseForTest(double phase) {
  phase_ = clamp01(phase);
  fixedPhase_ = true;
  timer_.stop();
  update();
}

QSize ScannerWidget::sizeHint() const {
  return QSize(760, 330);
}

QSize ScannerWidget::minimumSizeHint() const {
  return QSize(420, 250);
}

void ScannerWidget::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  updateTimer();
}

void ScannerWidget::hideEvent(QHideEvent* event) {
  timer_.stop();
  QWidget::hideEvent(event);
}

bool ScannerWidget::shouldAnimate() const {
  return state_ == State::Scanning || state_ == State::Validating;
}

void ScannerWidget::updateTimer() {
  const bool reduceMotion = qEnvironmentVariableIntValue(
    "FACE_UNLOCK_REDUCE_MOTION"
  ) == 1;
  if (isVisible() && shouldAnimate() && !fixedPhase_ && !reduceMotion) {
    if (!timer_.isActive()) timer_.start();
  } else {
    timer_.stop();
  }
}

QString ScannerWidget::stateLabel() const {
  switch (state_) {
    case State::Idle: return QStringLiteral("LOCAL SCANNER IDLE");
    case State::Scanning: return QStringLiteral("BUILDING FACE PROFILE");
    case State::Validating: return QStringLiteral("VALIDATING PROFILE");
    case State::Complete: return QStringLiteral("PROFILE SEALED");
    case State::Error: return QStringLiteral("SCAN PAUSED");
  }
  return QStringLiteral("LOCAL SCANNER");
}

void ScannerWidget::buildPointCloud() {
  points_.clear();
  points_.reserve(760);

  constexpr int columns = 31;
  constexpr int rows = 37;
  for (int row = 0; row < rows; ++row) {
    const double y = -1.08 + 2.16 * row / static_cast<double>(rows - 1);
    for (int column = 0; column < columns; ++column) {
      const double x = -0.86 + 1.72 * column /
        static_cast<double>(columns - 1);

      const double jawWidth = 0.78 - 0.18 * std::max(0.0, y - 0.34);
      const double ellipse = (x * x) / (jawWidth * jawWidth) +
        (y * y) / (1.08 * 1.08);
      if (ellipse > 1.0) continue;
      const bool eyeGap = std::abs(y + 0.20) < 0.035 &&
        std::abs(std::abs(x) - 0.27) < 0.095;
      const bool mouthGap = std::abs(y - 0.47) < 0.025 &&
        std::abs(x) < 0.19;
      if (eyeGap || mouthGap) continue;

      const double dome = std::sqrt(std::max(0.0, 1.0 - ellipse));
      const double nose = 0.42 * gaussian(x, y, 0.0, 0.02, 0.055);
      const double eyeLeft = 0.18 * gaussian(x, y, -0.27, -0.20, 0.028);
      const double eyeRight = 0.18 * gaussian(x, y, 0.27, -0.20, 0.028);
      const double mouth = 0.10 * gaussian(x, y, 0.0, 0.48, 0.045);
      const double cheekLeft = 0.11 * gaussian(x, y, -0.37, 0.14, 0.12);
      const double cheekRight = 0.11 * gaussian(x, y, 0.37, 0.14, 0.12);
      points_.push_back(FacePoint{
        x,
        y,
        0.72 * dome + nose + cheekLeft + cheekRight -
          eyeLeft - eyeRight - mouth,
      });
    }
  }
}

void ScannerWidget::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event)

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, false);

  const QRectF bounds = rect();
  const QColor background(5, 10, 18);
  const QColor panel(8, 18, 29);
  const QColor grid(37, 67, 82, 90);
  const QColor cyan(77, 235, 218);
  const QColor blue(69, 145, 255);
  const QColor magenta(225, 86, 255);
  const QColor muted(135, 162, 174);
  const QColor danger(255, 104, 112);
  const QColor success(115, 255, 177);

  painter.fillRect(bounds, background);
  painter.fillRect(bounds.adjusted(1, 1, -1, -1), panel);

  painter.setPen(QPen(grid, 1));
  const int gridStep = 28;
  for (int x = 0; x < width(); x += gridStep) painter.drawLine(x, 0, x, height());
  for (int y = 0; y < height(); y += gridStep) painter.drawLine(0, y, width(), y);

  const double activeProgress = progress_ / 100.0;
  double morph = 0.04;
  if (state_ == State::Scanning) morph = 0.16 + 0.72 * activeProgress;
  if (state_ == State::Validating) morph = 0.92;
  if (state_ == State::Complete) morph = 1.0;
  if (state_ == State::Error) morph = 0.12;
  morph = smoothstep(morph);

  const double pulse = 0.5 + 0.5 * std::sin(phase_ * 2.0 * kPi);
  const double yaw = shouldAnimate()
    ? 0.25 * std::sin(phase_ * 2.0 * kPi)
    : (state_ == State::Complete ? 0.08 : 0.0);
  const double cosine = std::cos(yaw);
  const double sine = std::sin(yaw);
  const QPointF center(width() * 0.5, height() * 0.54);
  const double scale = std::min(width() * 0.29, height() * 0.38);

  std::array<QVector<QPointF>, 5> buckets;
  for (auto& bucket : buckets) bucket.reserve(points_.size() / 4);

  for (const FacePoint& point : points_) {
    const double z = point.depth * morph;
    const double rx = point.x * cosine + z * sine;
    const double rz = -point.x * sine + z * cosine;
    const double perspective = 1.0 / (1.28 - 0.30 * rz);
    const double planeSpread = 1.0 + (1.0 - morph) * 0.42;
    const double px = center.x() + rx * scale * perspective * planeSpread;
    const double py = center.y() + point.y * scale * perspective;
    const int bucket = std::clamp(
      static_cast<int>((rz + 0.75) * 2.5), 0, 4
    );
    buckets[static_cast<std::size_t>(bucket)].append(QPointF(px, py));
  }

  const QColor activeColor = state_ == State::Complete
    ? success
    : (state_ == State::Error ? danger : cyan);
  for (std::size_t index = 0; index < buckets.size(); ++index) {
    const double depth = index / 4.0;
    QColor color = mixed(blue, activeColor, 0.30 + 0.70 * depth);
    color.setAlpha(105 + static_cast<int>(125 * depth));
    painter.setPen(QPen(color, 1.2 + depth * 1.8, Qt::SolidLine, Qt::RoundCap));
    painter.drawPoints(buckets[index]);
  }

  if (shouldAnimate()) {
    const double scanY = center.y() - scale + (2.0 * scale * phase_);
    QColor scan = mixed(cyan, magenta, pulse);
    scan.setAlpha(165);
    painter.setPen(QPen(scan, 1));
    painter.drawLine(
      QPointF(center.x() - scale * 0.95, scanY),
      QPointF(center.x() + scale * 0.95, scanY)
    );
  }

  painter.setRenderHint(QPainter::Antialiasing, true);
  const QRectF frame = bounds.adjusted(18, 18, -18, -18);
  const double corner = 24.0;
  painter.setPen(QPen(mixed(blue, cyan, 0.65), 1.5));
  painter.drawLine(frame.topLeft(), frame.topLeft() + QPointF(corner, 0));
  painter.drawLine(frame.topLeft(), frame.topLeft() + QPointF(0, corner));
  painter.drawLine(frame.topRight(), frame.topRight() - QPointF(corner, 0));
  painter.drawLine(frame.topRight(), frame.topRight() + QPointF(0, corner));
  painter.drawLine(frame.bottomLeft(), frame.bottomLeft() + QPointF(corner, 0));
  painter.drawLine(frame.bottomLeft(), frame.bottomLeft() - QPointF(0, corner));
  painter.drawLine(frame.bottomRight(), frame.bottomRight() - QPointF(corner, 0));
  painter.drawLine(frame.bottomRight(), frame.bottomRight() - QPointF(0, corner));

  QFont labelFont = font();
  labelFont.setFamilies({QStringLiteral("Monospace")});
  labelFont.setStyleHint(QFont::Monospace);
  labelFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
  labelFont.setPointSizeF(std::max(8.0, labelFont.pointSizeF() - 1.0));
  painter.setFont(labelFont);
  painter.setPen(muted);
  painter.drawText(
    QRectF(28, 22, width() - 56, 24),
    Qt::AlignLeft | Qt::AlignVCenter,
    QStringLiteral("LOCAL / CPU / PRIVATE")
  );
  painter.drawText(
    QRectF(28, height() - 48, width() - 56, 24),
    Qt::AlignLeft | Qt::AlignVCenter,
    stateLabel()
  );
  painter.setPen(activeColor);
  painter.drawText(
    QRectF(28, height() - 48, width() - 56, 24),
    Qt::AlignRight | Qt::AlignVCenter,
    QStringLiteral("%1%").arg(progress_, 3, 10, QLatin1Char('0'))
  );

  QFont guidanceFont = labelFont;
  guidanceFont.setPointSizeF(labelFont.pointSizeF() + 1.5);
  guidanceFont.setBold(true);
  painter.setFont(guidanceFont);
  painter.setPen(activeColor);
  painter.drawText(
    QRectF(28, 50, width() - 56, 30),
    Qt::AlignHCenter | Qt::AlignVCenter,
    guidance_
  );
}

bool renderScannerPreview(const QString& path) {
  if (path.isEmpty()) return false;
  ScannerWidget scanner;
  scanner.resize(960, 540);
  scanner.setScannerState(ScannerWidget::State::Scanning);
  scanner.setProgress(68);
  scanner.setGuidance(QStringLiteral("TURN SLIGHTLY LEFT"));
  scanner.setAnimationPhaseForTest(0.125);

  QImage image(scanner.size(), QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::transparent);
  QPainter painter(&image);
  scanner.render(&painter);
  painter.end();
  return image.save(path, "PNG");
}

int scannerWidgetSelfTest() {
  ScannerWidget scanner;
  if (scanner.minimumSizeHint().width() < 320) return 1;
  scanner.setScannerState(ScannerWidget::State::Validating);
  scanner.setProgress(140);
  scanner.setGuidance(QStringLiteral("validation"));
  scanner.setAnimationPhaseForTest(0.5);

  QImage image(640, 360, QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::transparent);
  scanner.resize(image.size());
  QPainter painter(&image);
  scanner.render(&painter);
  painter.end();

  if (image.isNull()) return 2;
  if (image.pixelColor(image.width() / 2, image.height() / 2).alpha() == 0) {
    return 3;
  }
  return 0;
}

int scannerWidgetBenchmark(int frames) {
  if (frames < 1 || frames > 10000) return 2;
  ScannerWidget scanner;
  scanner.resize(960, 540);
  scanner.setScannerState(ScannerWidget::State::Scanning);
  scanner.setProgress(68);
  scanner.setGuidance(QStringLiteral("TURN SLIGHTLY LEFT"));

  QImage image(scanner.size(), QImage::Format_ARGB32_Premultiplied);
  QElapsedTimer elapsed;
  elapsed.start();
  for (int frame = 0; frame < frames; ++frame) {
    scanner.setAnimationPhaseForTest(
      frame / static_cast<double>(std::max(1, frames - 1))
    );
    image.fill(Qt::transparent);
    QPainter painter(&image);
    scanner.render(&painter);
  }
  const double totalMs = elapsed.nsecsElapsed() / 1000000.0;
  std::cout << "scanner_benchmark_status: ok\n"
            << "frames: " << frames << '\n'
            << "width: " << image.width() << '\n'
            << "height: " << image.height() << '\n'
            << "total_ms: " << totalMs << '\n'
            << "mean_ms_per_frame: " << totalMs / frames << '\n'
            << "active_fps_cap: " << 1000 / kAnimationIntervalMs << '\n';
  return 0;
}

}  // namespace face_unlock_gui
