#include "./materialsnackbar.hpp"
#include "../Core/paintutil.hpp"
#include <QFontMetricsF>
#include <algorithm>
#include <cmath>

namespace {
constexpr qreal kMinW = 344.0;
constexpr qreal kMaxW = 672.0;
constexpr qreal kSideMargin = 16.0;
constexpr qreal kPadL = 16.0;
constexpr qreal kPadR = 16.0;
constexpr qreal kActionPad = 12.0;
constexpr qreal kActionGap = 8.0;
constexpr qreal kActionEdge = 8.0;
constexpr qreal kActionH = 40.0;
constexpr qreal kOneLineH = 48.0;
constexpr qreal kTwoLineH = 68.0;
constexpr qreal kLineH = 20.0;
constexpr qreal kTwoLinePad = 14.0;
constexpr qreal kRise = 24.0;
constexpr qreal kShownEps = 0.001;
constexpr int kSlotLine = 0;
constexpr int kSlotAction = 2;

QStringList wrap(const QFontMetricsF &fm, const QString &text, qreal w,
                 int maxLines) {
  QStringList out;
  if (w <= 8.0) {
    out << text;
    return out;
  }
  QString cur;
  const QStringList words = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
  for (QString word : words) {
    if (!cur.isEmpty()) {
      const QString t = cur + QLatin1Char(' ') + word;
      if (fm.horizontalAdvance(t) <= w) {
        cur = t;
        continue;
      }
      out << cur;
      cur.clear();
    }
    while (fm.horizontalAdvance(word) > w && word.size() > 1) {
      int k = 1;
      while (k < word.size() - 1 && fm.horizontalAdvance(word.left(k + 1)) <= w)
        ++k;
      out << word.left(k);
      word = word.mid(k);
    }
    cur = word;
  }
  if (!cur.isEmpty())
    out << cur;
  if (out.isEmpty())
    out << QString();
  if (out.size() > maxLines) {
    const QString tail = out.mid(maxLines - 1).join(QLatin1Char(' '));
    out = out.mid(0, maxLines);
    out.last() = fm.elidedText(tail, Qt::ElideRight, w);
  }
  return out;
}
} // namespace

MaterialSnackbar::MaterialSnackbar(QQuickItem *parent)
    : SgMaterialItem(parent, 420.0, 0.8) {
  setAcceptedMouseButtons(Qt::LeftButton);
  m_timer.setSingleShot(true);
  connect(&m_timer, &QTimer::timeout, this, [this] {
    if (!m_open)
      return;
    setOpen(false);
    emit dismissed();
  });
}

void MaterialSnackbar::componentComplete() {
  SgMaterialItem::componentComplete();
  retarget(true);
}

void MaterialSnackbar::geometryChange(const QRectF &n, const QRectF &o) {
  SgMaterialItem::geometryChange(n, o);
  if (n.size() != o.size())
    update();
}

QFont MaterialSnackbar::bodyFont() const {
  return Kit::styledFont(m_font, 14.0, QFont::Normal);
}

QFont MaterialSnackbar::actionFont() const {
  return Kit::styledFont(m_font, 14.0, QFont::Medium);
}

const QStringList &MaterialSnackbar::lines(qreal avail) const {
  const QString key = m_message + QLatin1Char('|') + QString::number(avail) +
                      QLatin1Char('|') + m_font.key();
  if (key != m_linesKey) {
    m_linesKey = key;
    m_lines = m_message.isEmpty()
                  ? QStringList()
                  : wrap(QFontMetricsF(bodyFont()), m_message, avail, 2);
  }
  return m_lines;
}

MaterialSnackbar::Layout MaterialSnackbar::layout() const {
  Layout l;
  const qreal cw =
      std::min(kMaxW, std::min(std::max(kMinW, width() - 2.0 * kSideMargin),
                               std::max(0.0, width() - 2.0 * kSideMargin)));
  qreal aw = 0.0;
  if (!m_action.isEmpty())
    aw = std::ceil(QFontMetricsF(actionFont()).horizontalAdvance(m_action)) +
         2.0 * kActionPad;
  l.textAvail = std::max(
      0.0, cw - kPadL - (aw > 0.0 ? aw + kActionGap + kActionEdge : kPadR));
  l.twoLine = lines(l.textAvail).size() > 1;
  const qreal ch = l.twoLine ? kTwoLineH : kOneLineH;
  const qreal p = std::clamp(m_prog.x, 0.0, 1.0);
  const qreal x = (width() - cw) / 2.0;
  const qreal y = height() - m_bottomMargin - ch + (1.0 - p) * kRise;
  l.card = QRectF(x, y, cw, ch);
  if (aw > 0.0)
    l.action = QRectF(l.card.right() - kActionEdge - aw,
                      l.card.center().y() - kActionH / 2.0, aw, kActionH);
  return l;
}

bool MaterialSnackbar::overAction(const QPointF &p) const {
  const Layout l = layout();
  return !l.action.isEmpty() && l.action.adjusted(-4, -4, 4, 4).contains(p);
}

void MaterialSnackbar::retarget(bool immediate) {
  m_prog.go(m_open ? 1.0 : 0.0, immediate);
  if (immediate) {
    m_settled = true;
    update();
    return;
  }
  animate();
}

void MaterialSnackbar::restartTimer() {
  m_timer.stop();
  if (m_open && m_activeDuration > 0)
    m_timer.start(m_activeDuration);
}

bool MaterialSnackbar::advance(qreal dt) {
  const bool active = step(m_prog, dt);
  if (!active && !m_settled) {
    m_settled = true;
    if (m_open)
      emit opened();
    else
      emit closed();
  }
  return active;
}

void MaterialSnackbar::setOpen(bool v) {
  if (!assign(m_open, v, &MaterialSnackbar::openChanged))
    return;
  m_settled = false;
  m_cardPress = false;
  m_actionPress = false;
  if (v)
    restartTimer();
  else
    m_timer.stop();
  retarget();
}

void MaterialSnackbar::show(const QString &message, const QString &actionText,
                            int duration) {
  m_activeDuration = duration >= 0 ? duration : m_duration;
  setMessage(message);
  setActionText(actionText);
  if (m_open) {
    restartTimer();
    update();
    return;
  }
  setOpen(true);
}

void MaterialSnackbar::setMessage(const QString &v) {
  assign(m_message, v, &MaterialSnackbar::messageChanged);
}
void MaterialSnackbar::setActionText(const QString &v) {
  assign(m_action, v, &MaterialSnackbar::actionTextChanged);
}
void MaterialSnackbar::setDuration(int v) {
  v = std::max(0, v);
  if (assign(m_duration, v, &MaterialSnackbar::durationChanged))
    m_activeDuration = v;
}
void MaterialSnackbar::setBottomMargin(qreal v) {
  assign(m_bottomMargin, v, &MaterialSnackbar::bottomMarginChanged);
}
void MaterialSnackbar::setRadius(qreal v) {
  assign(m_radius, std::max(0.0, v), &MaterialSnackbar::radiusChanged);
}
void MaterialSnackbar::setFont(const QFont &v) {
  assign(m_font, v, &MaterialSnackbar::fontChanged);
}
void MaterialSnackbar::setColor(const QColor &v) {
  assign(m_color, v, &MaterialSnackbar::colorChanged);
}
void MaterialSnackbar::setTextColor(const QColor &v) {
  assign(m_textColor, v, &MaterialSnackbar::textColorChanged);
}
void MaterialSnackbar::setActionColor(const QColor &v) {
  assign(m_actionColor, v, &MaterialSnackbar::actionColorChanged);
}

void MaterialSnackbar::mousePressEvent(QMouseEvent *e) {
  if (!isEnabled() || !m_open) {
    e->ignore();
    return;
  }
  const QPointF p = e->position();
  if (!layout().card.contains(p)) {
    e->ignore();
    return;
  }
  m_cardPress = true;
  m_actionPress = overAction(p);
  m_timer.stop();
  update();
  e->accept();
}

void MaterialSnackbar::mouseReleaseEvent(QMouseEvent *e) {
  if (!m_cardPress)
    return;
  const bool act = m_actionPress && overAction(e->position());
  m_cardPress = false;
  m_actionPress = false;
  update();
  if (act) {
    emit actionClicked();
    setOpen(false);
  } else {
    restartTimer();
  }
  e->accept();
}

void MaterialSnackbar::mouseUngrabEvent() {
  if (!m_cardPress)
    return;
  m_cardPress = false;
  m_actionPress = false;
  restartTimer();
  update();
}

void MaterialSnackbar::build(Kit::Mesh &m) {
  const qreal p = std::clamp(m_prog.x, 0.0, 1.0);
  if (width() <= 0.0 || height() <= 0.0 || p <= kShownEps)
    return;

  const Layout l = layout();
  const QRectF card(m.snap(l.card.left()), m.snap(l.card.top()), l.card.width(),
                    l.card.height());
  const QPointF d = card.topLeft() - l.card.topLeft();

  m.fillRoundedRect(card, m_radius, m_radius, m_radius, m_radius,
                    Kit::withAlpha(m_color, m_color.alphaF() * p));

  const qreal ta = std::round(p * 8.0) / 8.0;
  if (ta <= 0.0)
    return;
  auto fa = [ta](const QColor &c) {
    return Kit::withAlpha(c, c.alphaF() * ta);
  };

  const QStringList &ls = lines(l.textAvail);
  const QFont bf = bodyFont();
  const qreal x = card.left() + kPadL;
  for (int i = 0; i < ls.size(); ++i) {
    const qreal cy = l.twoLine
                         ? card.top() + kTwoLinePad + i * kLineH + kLineH / 2.0
                         : card.center().y();
    textLayer(kSlotLine + i, bf, ls[i], fa(m_textColor), x, cy);
  }

  if (!l.action.isEmpty()) {
    const QRectF r = l.action.translated(d);
    const qreal layer = Kit::stateLayer(m_actionPress);
    if (layer > 0.0)
      m.fillRoundedRect(r, kActionH / 2.0, kActionH / 2.0, kActionH / 2.0,
                        kActionH / 2.0,
                        Kit::withAlpha(m_actionColor, layer * p));
    textLayer(kSlotAction, actionFont(), m_action, fa(m_actionColor),
              r.left() + kActionPad, r.center().y());
  }
}
