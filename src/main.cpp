#include <qpl/qpl.hpp>

struct field {
	qsf::text_field text_field;
	qsf::smooth_rectangle rect;
	qpl::hitbox dragging_hitbox;
	qpl::hitbox hitbox;
	constexpr static qpl::rgb rect_color = qpl::rgb::grey_shade(200);

	bool first_update = true;
	bool hovering = false;
	bool dragging = false;
	bool selected = false;

	qpl::hitbox get_hitbox() const {
		return this->hitbox;
	}

	void init() {
		this->text_field.set_font("helvetica");
		this->text_field.background_increase = { 10, 10 };
		this->text_field.set_text_character_size(40);
		this->text_field.background.set_outline_thickness(5.0f);
		this->text_field.background.set_outline_color(qpl::rgb::grey_shade(0));

		this->rect.set_color(this->rect_color);
		this->rect.set_slope_dimension(30);
		this->first_update = true;
	}
	void set_position(qpl::vec2 position) {
		auto delta = position - this->hitbox.position;
		this->move(delta);
	}
	void move(qpl::vec2 delta) {
		this->text_field.move(delta);
		this->rect.move(delta);
		this->dragging_hitbox.move(delta);
		this->hitbox.move(delta);
	}
	void update_background() {
		this->hitbox = this->text_field.get_background_hitbox().increased(20);
		this->hitbox.extend_up(30);
		this->rect.set_hitbox(this->hitbox);

		this->dragging_hitbox = this->hitbox;
		this->dragging_hitbox.set_height(50);
	}
	void update(const qsf::event_info& event, bool other_selected) {
		if (other_selected) {
			return;
		}
		event.update(this->text_field);
		if (this->first_update || this->text_field.just_changed()) {
			this->update_background();
		}
		this->hovering = this->dragging_hitbox.contains(event.mouse_position());

		if (event.left_mouse_clicked()) {
			if (this->hovering) {
				this->dragging = true;
				this->selected = true;
				this->rect.set_color(qpl::rgb(200, 200, 255));
			}
			else {
				this->selected = false;
				this->rect.set_color(this->rect_color);
			}
		}
		if (event.left_mouse_released()) {
			this->dragging = false;
		}

		if (this->dragging) {
			auto delta = event.delta_mouse_position();
			this->move(delta);
		}

		this->first_update = false;
	}

	void draw(qsf::draw_object& draw) const {
		draw.draw(this->rect);
		draw.draw(this->text_field);

		qsf::rectangle rect;
		rect.set_hitbox(this->dragging_hitbox);
		draw.draw(rect);
	}
};

struct fields {
	std::vector<field> fields;
	qpl::size selected_index = qpl::size_max;
	qpl::size copy_index = qpl::size_max;
	bool allow_view_drag = true;

	void init() {
		this->fields.resize(1u);
		for (auto& i : this->fields) {
			i.init();
			i.set_position({ 200, 200 });
		}
	}
	qpl::hitbox find_free_spot_for(qpl::hitbox hitbox) const {

		std::vector<std::pair<qpl::size, qpl::f64>> distances(this->fields.size());
		for (qpl::size i = 0u; i < this->fields.size(); ++i) {
			distances[i].first = i;
			distances[i].second = (hitbox.get_center() - this->fields[i].get_hitbox().get_center()).length();
		}
		qpl::sort(distances, [](auto a, auto b) {
			return a.first < b.first;
		});


		for (auto& field : distances) {
			auto field_hitbox = this->fields[field.first].get_hitbox();

			for (qpl::size i = 0u; i < 4u; ++i) {
				auto pos = field_hitbox.get_side_corner_left((i + 2) % 4);
				pos += qpl::vec_square4[(i + 2) % 4] * 20;
				hitbox.set_side_corner_left(i, pos);
				if (!hitbox.collides(field_hitbox)) {
					return hitbox;
				}
				else {
					qpl::println(hitbox.string(), " collides with ", field_hitbox.string());
				}
			}
		}
		throw qpl::exception("fields::find_free_spot_for(", hitbox.string(), ") : no spot found. this shouldn't be possible!");
		return {};
	}

	void update(const qsf::event_info& event) {
		if (this->selected_index != qpl::size_max) {
			event.update(this->fields[this->selected_index], false);
			for (qpl::size i = 0u; i < this->fields.size(); ++i) {
				if (i == this->selected_index) {
					continue;
				}
				event.update(this->fields[i], true);
			}
		}
		else {
			bool one_selected = false;
			for (qpl::size i = 0u; i < this->fields.size(); ++i) {
				event.update(this->fields[i], one_selected);

				if (this->fields[i].selected) {
					this->selected_index = i;
					one_selected = true;
				}
			}
		}


		if (event.key_holding(sf::Keyboard::LControl)) {
			if (event.key_single_pressed(sf::Keyboard::C)) {
				this->copy_index = this->selected_index;
			}
			if (event.key_pressed(sf::Keyboard::V)) {
				if (this->copy_index != qpl::size_max) {
					auto hitbox = this->fields[this->copy_index].get_hitbox();
					hitbox.set_center(event.mouse_position());

					bool collides = false;
					for (auto& i : this->fields) {
						if (hitbox.collides(i.get_hitbox())) {
							collides = true;
							break;
						}
					}
					if (collides) {
						qpl::println("collides!");
						hitbox = this->find_free_spot_for(hitbox);
					}

					this->fields.push_back(this->fields[this->copy_index]);
					this->fields.back().set_position(hitbox.position);
					this->fields.back().selected = false;
					this->fields.back().dragging = false;
					this->fields.back().hovering = false;
				}
			}
		}

		bool one_hovering = false;
		this->allow_view_drag = true;
		for (auto& i : this->fields) {
			if (i.text_field.has_focus() || i.dragging) {
				this->allow_view_drag = false;
			}
			if (i.hovering) {
				one_hovering = true;
			}
		}

		if (one_hovering) {
			qpl::winsys::set_cursor_hand();
		}
		else {
			qpl::winsys::set_cursor_normal();
		}
	}
	void draw(qsf::draw_object& draw) const {

		if (this->selected_index != qpl::size_max) {
			for (qpl::size i = 0u; i < this->fields.size(); ++i) {
				if (i == this->selected_index) {
					continue;
				}
				draw.draw(this->fields[i]);
			}
			draw.draw(this->fields[this->selected_index]);
		}
		else {
			draw.draw(this->fields);
		}
		for (auto& i : this->fields) {

			qsf::rectangle rect;
			rect.set_hitbox(i.get_hitbox());
			rect.set_color(qpl::rgba::transparent());
			rect.set_outline_color(qpl::rgb::red());
			rect.set_outline_thickness(1.0f);
			draw.draw(rect);
		}
	}
};



struct main_state : qsf::base_state {
	void init() override {
		this->fields.init();
		this->call_on_resize();
	}
	void call_on_resize() override {
		this->view.set_hitbox(*this);
	}
	void updating() override {
		this->update(this->view);
		this->update(this->fields, this->view);

		this->view.allow_dragging = this->fields.allow_view_drag;
	}
	void drawing() override {
		this->draw(this->fields, this->view);
	}
	qsf::view_rectangle view;
	qpl::size side = 0u;
	fields fields;
};

int main() try {
	qsf::framework framework;
	framework.set_antialiasing_level(2);
	framework.set_title("QPL");
	framework.add_font("helvetica", "resources/Helvetica.ttf");
	framework.set_dimension({ 1400u, 950u });

	framework.add_state<main_state>();
	framework.game_loop();
}
catch (std::exception& any) {
	qpl::println("caught exception:\n", any.what());
	qpl::system_pause();
}