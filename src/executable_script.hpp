#pragma once
#include <qpl/qpl.hpp>
#include "config.hpp"

struct executable_script {
	qsf::smooth_rectangle background;
	qsf::smooth_rectangle checkmark_box;
	qsf::smooth_corner corner;
	qsf::vertex_array checkmark;
	qpl::hitbox hitbox;
	bool hovering = false;
	bool clicked = false;
	qpl::animation checkmark_hovering_animation;
	constexpr static qpl::rgb checkmark_color = qpl::rgb(138, 226, 138);
	constexpr static qpl::rgb checkmark_box_color = qpl::rgb::grey_shade(30);

	executable_script() {
		this->hitbox.set_dimension({ 130, 130 });
		this->hitbox.set_center({ 0, 0 });

		this->background.set_hitbox(this->hitbox.extended_up(60));
		this->background.set_slope_dimension(config::widget_background_slope_dimension);
		this->background.round_corners[0] = false;
		this->background.round_corners[1] = false;

		this->checkmark_box.set_dimension({ 90, 90 });
		this->checkmark_box.set_center(this->hitbox.get_center());
		this->checkmark_box.set_slope_dimension(config::widget_slope_dimension);
		this->checkmark_box.set_color(this->checkmark_box_color);
		this->checkmark_box.set_outline_thickness(5.0f);
		this->checkmark_box.set_outline_color(qpl::rgb::black());

		this->checkmark.resize(3u);
		this->checkmark.set_primitive_type(sf::PrimitiveType::Triangles);
		this->checkmark[0].position = qpl::vec(-1, -1) * 20;
		this->checkmark[1].position = qpl::vec(0.75, 0) * 20;
		this->checkmark[2].position = qpl::vec(-1, 1) * 20;

		for (auto& i : this->checkmark) {
			i.color = this->checkmark_color;
			i.position += qpl::vec(5, 0);
		}

		this->corner.set_dimension({ 40, 40 });
		this->corner.set_slope_point_count(40);
		this->corner.invert();
		this->corner.set_rotation(qpl::pi);

		auto h = this->corner.get_hitbox();
		h.set_top_right(this->hitbox.get_top_left() + qpl::vec(0, 15));
		this->corner.set_hitbox(h);

		this->checkmark_hovering_animation.set_duration(0.2);
	}

	qpl::hitbox get_hitbox() const {
		return this->hitbox;
	}

	void set_background_color(qpl::rgb color) {
		this->background.set_color(color);
		this->corner.set_color(color);
	}
	void move(qpl::vec2 delta) {
		this->background.move(delta);
		this->checkmark.move(delta);
		this->checkmark_box.move(delta);
		this->corner.move(delta);
		this->hitbox.move(delta);
	}
	void update_position(qpl::hitbox hitbox) {
		qpl::hitbox new_hitbox = this->hitbox;
		new_hitbox.set_right_top(hitbox.get_bottom_right() - qpl::vec(0, 15));
		auto diff = new_hitbox.position - this->get_hitbox().position;
		this->move(diff);
	}
	void update(const qsf::event_info& event) {
		this->hovering = this->hitbox.contains(event.mouse_position());
		this->clicked = this->hovering && event.left_mouse_clicked();

		if (this->hovering) {
			this->checkmark_hovering_animation.go_forwards();
		}
		else {
			this->checkmark_hovering_animation.go_backwards();
		}

		this->checkmark_hovering_animation.update(event);
		if (this->checkmark_hovering_animation.is_running()) {
			auto p = this->checkmark_hovering_animation.get_curve_progress();
			auto color = this->checkmark_color.lighted(p / 2);
			this->checkmark.set_color(color);

			color = this->checkmark_box_color.darkened(p);
			this->checkmark_box.set_color(color);
		}
	}
	void draw(qsf::draw_object& draw) const {
		draw.draw(this->background);
		draw.draw(this->checkmark_box);
		draw.draw(this->checkmark);
		draw.draw(this->corner);
	}
};
