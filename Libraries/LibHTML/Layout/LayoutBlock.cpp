#include <LibGUI/GPainter.h>
#include <LibHTML/DOM/Element.h>
#include <LibHTML/Layout/LayoutBlock.h>
#include <LibHTML/Layout/LayoutInline.h>

LayoutBlock::LayoutBlock(const Node* node, NonnullRefPtr<StyleProperties> style_properties)
    : LayoutNode(node, move(style_properties))
{
}

LayoutBlock::~LayoutBlock()
{
}

LayoutNode& LayoutBlock::inline_wrapper()
{
    if (!last_child() || !last_child()->is_block() || last_child()->node() != nullptr) {
        append_child(adopt(*new LayoutBlock(nullptr, style())));
    }
    return *last_child();
}

void LayoutBlock::layout()
{
    compute_width();
    compute_position();

    if (children_are_inline())
        layout_inline_children();
    else
        layout_block_children();

    compute_height();
}

void LayoutBlock::layout_block_children()
{
    ASSERT(!children_are_inline());
    int content_height = 0;
    for_each_child([&](auto& child) {
        child.layout();
        content_height = child.rect().bottom() + child.box_model().full_margin().bottom - rect().top();
    });
    rect().set_height(content_height);
}

void LayoutBlock::layout_inline_children()
{
    ASSERT(children_are_inline());
    m_line_boxes.clear();
    for_each_child([&](auto& child) {
        ASSERT(child.is_inline());
        static_cast<LayoutInline&>(child).split_into_lines(*this);
    });

    int content_height = 0;

    for (auto& line_box : m_line_boxes) {
        int max_height = 0;
        for (auto& fragment : line_box.fragments()) {
            max_height = max(max_height, fragment.rect().height());
        }
        for (auto& fragment : line_box.fragments()) {
            // Vertically align everyone's bottom to the line.
            // FIXME: Support other kinds of vertical alignment.
            fragment.rect().set_x(rect().x() + fragment.rect().x());
            fragment.rect().set_y(rect().y() + content_height + (max_height - fragment.rect().height()));
        }

        content_height += max_height;
    }

    rect().set_height(content_height);
}

void LayoutBlock::compute_width()
{
    auto& style_properties = this->style();

    auto auto_value = Length();
    auto zero_value = Length(0, Length::Type::Absolute);
    auto width = style_properties.length_or_fallback("width", auto_value);
    auto margin_left = style_properties.length_or_fallback("margin-left", zero_value);
    auto margin_right = style_properties.length_or_fallback("margin-right", zero_value);
    auto border_left = style_properties.length_or_fallback("border-left", zero_value);
    auto border_right = style_properties.length_or_fallback("border-right", zero_value);
    auto padding_left = style_properties.length_or_fallback("padding-left", zero_value);
    auto padding_right = style_properties.length_or_fallback("padding-right", zero_value);

#ifdef HTML_DEBUG
    dbg() << " Left: " << margin_left << "+" << border_left << "+" << padding_left;
    dbg() << "Right: " << margin_right << "+" << border_right << "+" << padding_right;
#endif

    int total_px = 0;
    for (auto& value : { margin_left, border_left, padding_left, width, padding_right, border_right, margin_right }) {
        total_px += value.to_px();
    }

#ifdef HTML_DEBUG
    dbg() << "Total: " << total_px;
#endif

    // 10.3.3 Block-level, non-replaced elements in normal flow
    // If 'width' is not 'auto' and 'border-left-width' + 'padding-left' + 'width' + 'padding-right' + 'border-right-width' (plus any of 'margin-left' or 'margin-right' that are not 'auto') is larger than the width of the containing block, then any 'auto' values for 'margin-left' or 'margin-right' are, for the following rules, treated as zero.
    if (width.is_auto() && total_px > containing_block()->rect().width()) {
        if (margin_left.is_auto())
            margin_left = zero_value;
        if (margin_right.is_auto())
            margin_right = zero_value;
    }

    // 10.3.3 cont'd.
    auto underflow_px = containing_block()->rect().width() - total_px;

    if (width.is_auto()) {
        if (margin_left.is_auto())
            margin_left = zero_value;
        if (margin_right.is_auto())
            margin_right = zero_value;
        if (underflow_px >= 0) {
            width = Length(underflow_px, Length::Type::Absolute);
        } else {
            width = zero_value;
            margin_right = Length(margin_right.to_px() + underflow_px, Length::Type::Absolute);
        }
    } else {
        if (!margin_left.is_auto() && !margin_right.is_auto()) {
            margin_right = Length(margin_right.to_px() + underflow_px, Length::Type::Absolute);
        } else if (!margin_left.is_auto() && margin_right.is_auto()) {
            margin_right = Length(underflow_px, Length::Type::Absolute);
        } else if (margin_left.is_auto() && !margin_right.is_auto()) {
            margin_left = Length(underflow_px, Length::Type::Absolute);
        } else { // margin_left.is_auto() && margin_right.is_auto()
            auto half_of_the_underflow = Length(underflow_px / 2, Length::Type::Absolute);
            margin_left = half_of_the_underflow;
            margin_right = half_of_the_underflow;
        }
    }

    rect().set_width(width.to_px());
    box_model().margin().left = margin_left;
    box_model().margin().right = margin_right;
    box_model().border().left = border_left;
    box_model().border().right = border_right;
    box_model().padding().left = padding_left;
    box_model().padding().right = padding_right;
}

void LayoutBlock::compute_position()
{
    auto& style_properties = this->style();

    auto auto_value = Length();
    auto zero_value = Length(0, Length::Type::Absolute);

    auto width = style_properties.length_or_fallback("width", auto_value);

    box_model().margin().top = style_properties.length_or_fallback("margin-top", zero_value);
    box_model().margin().bottom = style_properties.length_or_fallback("margin-bottom", zero_value);
    box_model().border().top = style_properties.length_or_fallback("border-top", zero_value);
    box_model().border().bottom = style_properties.length_or_fallback("border-bottom", zero_value);
    box_model().padding().top = style_properties.length_or_fallback("padding-top", zero_value);
    box_model().padding().bottom = style_properties.length_or_fallback("padding-bottom", zero_value);
    rect().set_x(containing_block()->rect().x() + box_model().margin().left.to_px() + box_model().border().left.to_px() + box_model().padding().left.to_px());

    int top_border = -1;
    if (previous_sibling() != nullptr) {
        auto& previous_sibling_rect = previous_sibling()->rect();
        auto& previous_sibling_style = previous_sibling()->box_model();
        top_border = previous_sibling_rect.y() + previous_sibling_rect.height();
        top_border += previous_sibling_style.full_margin().bottom;
    } else {
        top_border = containing_block()->rect().y();
    }
    rect().set_y(top_border + box_model().full_margin().top);
}

void LayoutBlock::compute_height()
{
    auto& style_properties = this->style();

    auto height_property = style_properties.property("height");
    if (!height_property.has_value())
        return;
    auto height_length = height_property.value()->to_length();
    if (height_length.is_absolute())
        rect().set_height(height_length.to_px());
}

void LayoutBlock::render(RenderingContext& context)
{
    LayoutNode::render(context);

    // FIXME: position this properly
    if (style().string_or_fallback("display", "block") == "list-item") {
        Rect bullet_rect {
            rect().x() - 8,
            rect().y() + 4,
            3,
            3
        };
        context.painter().fill_rect(bullet_rect, Color::Black);
    }

    if (children_are_inline()) {
        for (auto& line_box : m_line_boxes) {
            for (auto& fragment : line_box.fragments()) {
                fragment.render(context);
            }
        }
    }
}

bool LayoutBlock::children_are_inline() const
{
    return first_child() && !first_child()->is_block();
}

HitTestResult LayoutBlock::hit_test(const Point& position) const
{
    if (!children_are_inline())
        return LayoutNode::hit_test(position);

    HitTestResult result;
    for (auto& line_box : m_line_boxes) {
        for (auto& fragment : line_box.fragments()) {
            if (fragment.rect().contains(position)) {
                return { fragment.layout_node() };
            }
        }
    }
    return {};
}
