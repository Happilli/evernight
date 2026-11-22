#include "./materialdialog.hpp"
#include "../Core/paintutil.hpp"
#include <QFontMetricsF>
#include <algorithm>
#include <cmath>

namespace {
constexpr qreal kMinW = 280.0;
constexpr qreal kMaxW = 560.0;
constexpr qreal kMargin = 24.0;
constexpr qreal kPad = 24.0;
constexpr qreal kRadius = 28.0;
constexpr qreal kTitleLh = 32.0;
constexpr qreal kTextLh = 20.0;
constexpr qreal kGap = 16.0;
constexpr qreal kBtnH = 40.0;
constexpr qreal kBtnPad = 12.0;
constexpr qreal kBtnGap = 8.0;
constexpr qreal kScrimAlpha = 0.32;
constexpr qreal kRise = 16.0;
constexpr int kMaxLines = 8;
constexpr int kSlotTitle = 0;
constexpr int kSlotCancel = 1;
constexpr int kSlotConfirm = 2;
constexpr int kSlotText = 3;

QStringList wrap(const QFontMetricsF &fm, const QString &text, qreal w,
                 int maxLines) {
  QStringList out;
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
  if (out.size() > maxLines) {
    out = out.mid(0, maxLines);
    out.last() =
        fm.elidedText(out.last() + QStringLiteral("\u2026"), Qt::ElideRight, w);
  }
  return out;
}
} // namespace

MaterialDialog::MaterialDialog(QQuickItem *parent)
    : SgMaterialItem(parent, 380.0, 1.0), m_panel(new QQuickItem(this)) {
  setAcceptedMouseButtons(Qt::LeftButton);
  m_panel->setVisible(false);
}

QQmlListProperty<QObject> MaterialDialog::content() {
  return QQmlListProperty<QObject>(this, this, &MaterialDialog::appendContent,
                                   &MaterialDialog::contentCount,
                                   &MaterialDialog::contentAt, nullptr);
}

void MaterialDialog::appendContent(QQmlListProperty<QObject> *l, QObject *o) {
  auto *self = static_cast<MaterialDialog *>(l->data);
  if (auto *item = qobject_cast<QQuickItem *>(o))
    item->setParentItem(self->m_panel);
  else
    o->setParent(self);
}

qsizetype MaterialDialog::contentCount(QQmlListProperty<QObject> *l) {
  return static_cast<MaterialDialog *>(l->data)->m_panel->childItems().size();
}

QObject *MaterialDialog::contentAt(QQmlListProperty<QObject> *l, qsizetype i) {
  return static_cast<MaterialDialog *>(l->data)->m_panel->childItems().at(i);
}

void MaterialDialog::componentComplete() {
  SgMaterialItem::componentComplete();
  retarget(true);
}

void MaterialDialog::geometryChange(const QRectF &n, const QRectF &o) {
  SgMaterialItem::geometryChange(n, o);
  applyLayout();
  update();
}

QFont MaterialDialog::bodyFont() const {
  return Kit::styledFont(m_font, 14.0, QFont::Normal);
}

QFont MaterialDialog::actionFont() const {
  return Kit::styledFont(m_font, 14.0, QFont::Medium);
}

qreal MaterialDialog::contentExtent() const {
  qreal b = 0.0;
  for (const QQuickItem *c : m_panel->childItems())
    if (c->isVisible())
      b = std::max(b, c->y() + c->height());
  return b;
}

const QStringList &MaterialDialog::lines(qreal avail) const {
  const QString key = m_text + QLatin1Char('|') + QString::number(avail) +
                      QLatin1Char('|') + m_font.key();
  if (key != m_linesKey) {
    m_linesKey = key;
    m_lines = m_text.isEmpty()
                  ? QStringList()
                  : wrap(QFontMetricsF(bodyFont()), m_text, avail, kMaxLines);
  }
  return m_lines;
}

MaterialDialog::Layout MaterialDialog::layout() const {
  Layout l;
  const qreal cw = std::min(kMaxW, std::max(kMinW, width() - 2.0 * kMargin));
  const qreal inner = cw - 2.0 * kPad;
  const int nl = int(lines(inner).size());
  const qreal ch = contentExtent();

  qreal y = kPad;
  if (!m_title.isEmpty()) {
    l.titleY = y;
    y += kTitleLh;
  }
  if (nl > 0) {
    if (!m_title.isEmpty())
      y += kGap;
    l.textY = y;
    y += nl * kTextLh;
  }
  qreal panelY = y;
  if (ch > 0.0) {
    if (!m_title.isEmpty() || nl > 0)
      y += kGap;
    panelY = y;
    y += ch;
  }
  y += kPad;
  const qreal btnY = y;
  y += kBtnH + kPad;

  const qreal p = std::clamp(m_prog.x, 0.0, 1.0);
  const qreal x = (width() - cw) / 2.0;
  const qreal top = (height() - y) / 2.0 + (1.0 - p) * kRise;
  l.card = QRectF(x, top, cw, y);
  l.panel = QRectF(x + kPad, top + panelY, inner, ch);

  const QFontMetricsF fm(actionFont());
  qreal right = x + cw - kPad;
  if (!m_confirm.isEmpty()) {
    const qreal w = fm.horizontalAdvance(m_confirm) + 2.0 * kBtnPad;
    l.confirm = QRectF(right - w, top + btnY, w, kBtnH);
    right -= w + kBtnGap;
  }
  if (!m_cancel.isEmpty()) {
    const qreal w = fm.horizontalAdvance(m_cancel) + 2.0 * kBtnPad;
    l.cancel = QRectF(right - w, top + btnY, w, kBtnH);
  }
  return l;
}

int MaterialDialog::hit(const QPointF &p) const {
  const Layout l = layout();
  if (l.confirm.contains(p) && m_confirmEnabled)
    return Confirm;
  if (l.cancel.contains(p))
    return Cancel;
  return None;
}

void MaterialDialog::applyLayout() {
  const bool shown = m_open || m_prog.x > 0.001;
  m_panel->setVisible(shown);
  const Layout l = layout();
  m_panel->setPosition(l.panel.topLeft());
  m_panel->setSize(l.panel.size());
  m_panel->setOpacity(std::clamp(m_prog.x, 0.0, 1.0));
}

void MaterialDialog::retarget(bool immediate) {
  m_prog.go(m_open ? 1.0 : 0.0, immediate);
  if (immediate) {
    m_settled = true;
    applyLayout();
    update();
    return;
  }
  animate();
}

bool MaterialDialog::advance(qreal dt) {
  const bool active = step(m_prog, dt);
  applyLayout();
  if (!active && !m_settled) {
    m_settled = true;
    if (m_open)
      emit opened();
    else
      emit closed();
  }
  return active;
}

void MaterialDialog::setOpen(bool v) {
  if (!assign(m_open, v, &MaterialDialog::openChanged))
    return;
  m_settled = false;
  m_pressZone = None;
  m_scrimPress = false;
  retarget();
}

void MaterialDialog::accept() {
  if (!m_open || !m_confirmEnabled)
    return;
  emit accepted();
  setOpen(false);
}

void MaterialDialog::reject() {
  if (!m_open)
    return;
  emit rejected();
  setOpen(false);
}

void MaterialDialog::setTitle(const QString &v) {
  assign(m_title, v, &MaterialDialog::titleChanged);
}
void MaterialDialog::setText(const QString &v) {
  assign(m_text, v, &MaterialDialog::textChanged);
}
void MaterialDialog::setConfirmText(const QString &v) {
  assign(m_confirm, v, &MaterialDialog::confirmTextChanged);
}
void MaterialDialog::setCancelText(const QString &v) {
  assign(m_cancel, v, &MaterialDialog::cancelTextChanged);
}
void MaterialDialog::setDestructive(bool v) {
  assign(m_destructive, v, &MaterialDialog::destructiveChanged);
}
void MaterialDialog::setConfirmEnabled(bool v) {
  assign(m_confirmEnabled, v, &MaterialDialog::confirmEnabledChanged);
}
void MaterialDialog::setDismissOnScrim(bool v) {
  assign(m_dismissOnScrim, v, &MaterialDialog::dismissOnScrimChanged);
}
void MaterialDialog::setFont(const QFont &v) {
  assign(m_font, v, &MaterialDialog::fontChanged);
}
void MaterialDialog::setColor(const QColor &v) {
  assign(m_color, v, &MaterialDialog::colorChanged);
}
void MaterialDialog::setScrimColor(const QColor &v) {
  assign(m_scrimColor, v, &MaterialDialog::scrimColorChanged);
}
void MaterialDialog::setTitleColor(const QColor &v) {
  assign(m_titleColor, v, &MaterialDialog::titleColorChanged);
}
void MaterialDialog::setTextColor(const QColor &v) {
  assign(m_textColor, v, &MaterialDialog::textColorChanged);
}
void MaterialDialog::setActionColor(const QColor &v) {
  assign(m_actionColor, v, &MaterialDialog::actionColorChanged);
}
void MaterialDialog::setDangerColor(const QColor &v) {
  assign(m_dangerColor, v, &MaterialDialog::dangerColorChanged);
}

void MaterialDialog::activate(int zone) {
  if (zone == Confirm)
    accept();
  else if (zone == Cancel)
    reject();
}

void MaterialDialog::mousePressEvent(QMouseEvent *e) {
  if (!isEnabled() || !m_open) {
    e->ignore();
    return;
  }
  const QPointF p = e->position();
  if (!layout().card.contains(p)) {
    m_scrimPress = true;
    e->accept();
    return;
  }
  m_pressZone = hit(p);
  update();
  e->accept();
}

void MaterialDialog::mouseReleaseEvent(QMouseEvent *e) {
  if (m_scrimPress) {
    m_scrimPress = false;
    if (m_dismissOnScrim && !layout().card.contains(e->position()))
      reject();
    e->accept();
    return;
  }
  const int pressed = m_pressZone;
  m_pressZone = None;
  update();
  if (pressed != None && hit(e->position()) == pressed)
    activate(pressed);
  e->accept();
}

void MaterialDialog::mouseUngrabEvent() {
  m_scrimPress = false;
  m_pressZone = None;
  update();
}

void MaterialDialog::build(Kit::Mesh &m) {
  const qreal w = width(), h = height();
  const qreal p = std::clamp(m_prog.x, 0.0, 1.0);
  if (w <= 0.0 || h <= 0.0 || p <= 0.0)
    return;

  m.fillRoundedRect(QRectF(0.0, 0.0, w, h), 0.0, 0.0, 0.0, 0.0,
                    Kit::withAlpha(m_scrimColor, kScrimAlpha * p));

  const Layout l = layout();
  const QRectF card(m.snap(l.card.left()), m.snap(l.card.top()), l.card.width(),
                    l.card.height());
  m.fillRoundedRect(card, kRadius, kRadius, kRadius, kRadius,
                    Kit::withAlpha(m_color, m_color.alphaF() * p));

  const qreal ta = std::round(p * 8.0) / 8.0;
  if (ta <= 0.0)
    return;
  auto fa = [ta](const QColor &c) {
    return Kit::withAlpha(c, c.alphaF() * ta);
  };

  const qreal x = card.left() + kPad;
  if (!m_title.isEmpty()) {
    const QFont tf = Kit::styledFont(m_font, 24.0, QFont::Normal);
    const QString t = QFontMetricsF(tf).elidedText(m_title, Qt::ElideRight,
                                                   card.width() - 2.0 * kPad);
    textLayer(kSlotTitle, tf, t, fa(m_titleColor), x,
              card.top() + l.titleY + kTitleLh / 2.0);
  }

  const QStringList &ls = lines(card.width() - 2.0 * kPad);
  const QFont bf = bodyFont();
  for (int i = 0; i < ls.size(); ++i)
    textLayer(kSlotText + i, bf, ls[i], fa(m_textColor), x,
              card.top() + l.textY + i * kTextLh + kTextLh / 2.0);

  const QFont af = actionFont();
  auto button = [&](const QRectF &r, const QString &label, int zone, int slot,
                    const QColor &col) {
    if (r.isEmpty())
      return;
    const qreal layer = Kit::stateLayer(m_pressZone == zone);
    if (layer > 0.0)
      m.fillRoundedRect(r, kBtnH / 2.0, kBtnH / 2.0, kBtnH / 2.0, kBtnH / 2.0,
                        Kit::withAlpha(col, layer * p));
    textLayer(slot, af, label, fa(col), r.left() + kBtnPad, r.center().y());
  };
  QColor confirmCol = m_destructive ? m_dangerColor : m_actionColor;
  if (!m_confirmEnabled)
    confirmCol =
        Kit::withAlpha(confirmCol, confirmCol.alphaF() * Kit::kDisabledOpacity);
  button(l.cancel.translated(card.topLeft() - l.card.topLeft()), m_cancel,
         Cancel, kSlotCancel, m_actionColor);
  button(l.confirm.translated(card.topLeft() - l.card.topLeft()), m_confirm,
         Confirm, kSlotConfirm, confirmCol);
}
