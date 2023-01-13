#pragma once
#include <qpl/qpl.hpp>
#include "executable_script.hpp"

enum class widget_type {
	text,
	executable_script,
};

struct widget {
	qsf::text_field text;
	qsf::smooth_rectangle background;
	qpl::hitbox dragging_hitbox;
	qpl::hitbox hitbox;
	widget_type type = widget_type::text;

	std::unique_ptr<executable_script> executable_script;

	bool first_update = true;
	bool hovering = false;
	bool dragging = false;
	bool just_selected = false;

	constexpr static qpl::rgb background_color = qpl::rgb::grey_shade(100);

	qpl::hitbox get_hitbox() const {
		return this->hitbox;
	}

	widget() {

	}
	widget(const widget& other) {
		*this = other;
	}
	widget& operator=(const widget& other) {
		this->text = other.text;
		this->background = other.background;
		this->dragging_hitbox = other.dragging_hitbox;
		this->hitbox = other.hitbox;
		this->type = other.type;
		this->first_update = other.first_update;
		this->hovering = false;
		this->dragging = false;
		this->just_selected = false;

		if (other.executable_script) {
			this->executable_script = std::make_unique<::executable_script>(*other.executable_script);
		}
		return *this;
	}

	void save(qpl::save_state& state) const {
		state.save(this->text.wstring());
		state.save(this->hitbox.position);
		state.save(this->type);
	}
	bool load(qpl::save_state state) {
		std::wstring text_string;
		state.load(text_string);

		qpl::vec2f position;
		state.load(position);

		state.load(this->type);

		this->set_widget_type(this->type);
		this->set_position(position);
		this->text.set_string(text_string);
		return true;
	}
	void init() {
		this->text.set_font("helvetica");
		this->text.background_increase = { 20, 20 };
		this->text.set_text_character_size(40);
		this->text.background.set_outline_thickness(5.0f);
		this->text.background.set_outline_color(qpl::rgb::grey_shade(0));

		this->background.set_color(this->background_color);
		this->background.set_slope_dimension(30);
		this->set_position({ 0, 0 });
		this->first_update = true;

		this->type = widget_type::text;
		this->executable_script.reset();
	}

	void set_widget_type(::widget_type type) {
		this->type = type;

		if (this->type == widget_type::executable_script) {
			this->executable_script = std::make_unique<::executable_script>();
			this->executable_script->set_background_color(this->background_color);

			this->text.set_font("consola");
		}
	}

	void set_position(qpl::vec2 position) {
		auto delta = position - this->hitbox.position;
		this->move(delta);
	}
	void move(qpl::vec2 delta) {
		this->text.move(delta);
		this->background.move(delta);
		this->dragging_hitbox.move(delta);
		this->hitbox.move(delta);

		if (this->executable_script) {
			this->executable_script->move(delta);
		}
	}
	void update_background() {
		this->hitbox = this->text.get_background_hitbox().increased(20);
		this->hitbox.extend_up(30);
		this->background.set_hitbox(this->hitbox);
		this->background.set_color(this->background_color);

		this->dragging_hitbox = this->hitbox;
		this->dragging_hitbox.set_height(50);
		if (this->executable_script) {
			this->executable_script->update_position(this->hitbox);
		}
	}
	void set_background_color(qpl::rgb color) {
		this->background.set_color(color);
		if (this->executable_script) {
			this->executable_script->set_background_color(color);
		}
	}
	void update(const qsf::event_info& event, bool other_selected) {
		if (!other_selected) {
			event.update(this->text);
		}

		if (this->first_update || this->text.just_changed()) {
			this->update_background();
		}
		this->hovering = this->dragging_hitbox.contains(event.mouse_position());
		this->just_selected = false;
		if (event.left_mouse_clicked()) {
			if (this->hovering && !other_selected) {
				this->dragging = true;
				this->just_selected = true;
				this->set_background_color(qpl::rgb(200, 200, 255));
			}
			else {
				this->set_background_color(this->background_color);
			}
		}
		if (event.left_mouse_released()) {
			this->dragging = false;
		}

		if (this->dragging) {
			auto delta = event.delta_mouse_position();
			this->move(delta);
		}

		if (this->executable_script) {
			event.update(*this->executable_script);
		}

		this->first_update = false;
	}
	void draw(qsf::draw_object& draw) const {
		if (this->executable_script) {
			draw.draw(*this->executable_script);
		}
		draw.draw(this->background);
		draw.draw(this->text);
	}
};