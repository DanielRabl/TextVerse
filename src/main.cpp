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
	bool just_selected = false;

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
		this->rect.set_color(this->rect_color);

		this->dragging_hitbox = this->hitbox;
		this->dragging_hitbox.set_height(50);
	}
	void update(const qsf::event_info& event, bool other_selected) {
		if (!other_selected) {
			event.update(this->text_field);
		}

		if (this->first_update || this->text_field.just_changed()) {
			this->update_background();
		}
		this->hovering = this->dragging_hitbox.contains(event.mouse_position());
		this->just_selected = false;
		if (event.left_mouse_clicked()) {
			if (this->hovering && !other_selected) {
				this->dragging = true;
				this->just_selected = true;
				this->rect.set_color(qpl::rgb(200, 200, 255));
			}
			else {
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

	void save(qpl::save_state& state) const {
		state.save(this->text_field.wstring());
		state.save(this->hitbox.position);
	}
	void load(qpl::save_state state) {
		std::wstring text_string;
		state.load(text_string);
		state.load(this->hitbox.position);

		this->text_field.set_string(text_string);
	}

	void draw(qsf::draw_object& draw) const {
		draw.draw(this->rect);
		draw.draw(this->text_field);
	}
};

struct fields {
	std::vector<field> fields;
	std::list<qpl::size> draw_order;

	qpl::size selected_index = qpl::size_max;
	qpl::size copy_index = qpl::size_max;
	bool allow_view_drag = true;
	bool any_text_field_focus = false;
	bool turbo = false;


	void init() {

		this->fields.resize(1u);
		for (auto& i : this->fields) {
			i.init();
			i.set_position({ 200, 200 });
		}
		this->draw_order.push_back(0u);

		//this->load();
	}

	void save() {
		qpl::save_state state;
		state.save(this->fields.size());
		for (auto& i : this->fields) {
			state.save(i);
		}

		std::vector<qpl::size> order(this->fields.size());
		auto it = this->draw_order.begin();
		for (qpl::size i = 0u; i < this->fields.size(); ++i) {
			order[i] = *it;
			++it;
		}
		state.save(this->draw_order);
		

		qpl::write_data_file(state.get_finalized_string(), "data/session.dat");
	}
	void load() {
		auto data = qpl::filesys::read_file("data/session.dat");

		qpl::save_state state;
		state.set_string(data);

		qpl::size size;
		state.load(size);

		qpl::println("size = ", size);

		this->fields.resize(size);
		for (auto& i : this->fields) {
			i.init();
			state.load(i);
			qpl::println("string = ", i.text_field.string());
		}

		std::vector<qpl::size> order;
		state.load(order);
		this->draw_order.clear();
		for (auto& i : order) {
			this->draw_order.push_back(i);
			qpl::println("push_back : ", i);
		}

		qpl::println("DRAW ORDER : ", this->draw_order);
	}

	bool hitbox_collides_with_field(qpl::hitbox hitbox) const {
		for (auto& i : this->fields) {
			if (hitbox.collides(i.get_hitbox())) {
				return true;
			}
		}
		return false;
	}

	qpl::hitbox find_free_spot_for(qpl::hitbox hitbox) const {

		std::vector<std::pair<qpl::size, qpl::f64>> distances(this->fields.size());
		for (qpl::size i = 0u; i < this->fields.size(); ++i) {
			distances[i].first = i;
			distances[i].second = (hitbox.get_center() - this->fields[i].get_hitbox().get_center()).length();
		}
		qpl::sort(distances, [](auto a, auto b) {
			return a.second < b.second;
		});


		std::array<qpl::size, 4u> sides;
		for (qpl::size i = 0u; i < sides.size(); ++i) {
			sides[i] = i;
		}

		for (auto& field : distances) {
			auto field_hitbox = this->fields[field.first].get_hitbox();
			qpl::shuffle(sides);
			
			for (const auto& i : sides) {
				auto pos = field_hitbox.get_side_corner_left((i + 2) % 4);
				pos += qpl::vec_cross4[(i + 1) % 4] * 10;

				hitbox.set_side_corner_left(i, pos);
				if (!this->hitbox_collides_with_field(hitbox)) {
					return hitbox;
				}
			}
		}
		throw qpl::exception("fields::find_free_spot_for(", hitbox.string(), ") : no spot found. this shouldn't be possible!");
		return {};
	}

	void paste(qpl::vec2f position) {
		if (this->copy_index != qpl::size_max) {
			auto hitbox = this->fields[this->copy_index].get_hitbox();
			hitbox.set_center(position);

			if (this->hitbox_collides_with_field(hitbox)) {
				hitbox = this->find_free_spot_for(hitbox);
			}

			this->fields.push_back(this->fields[this->copy_index]);
			this->fields.back().set_position(hitbox.position);
			this->fields.back().update_background();
			this->draw_order.push_back(this->fields.size() - 1);
		}
	}

	void update_input(const qsf::event_info& event) {
		if (!this->any_text_field_focus) {
			if (event.key_holding(sf::Keyboard::LControl)) {

				if (this->turbo && event.key_holding(sf::Keyboard::V)) {
					this->paste(event.mouse_position());
				}
				if (event.key_single_pressed(sf::Keyboard::C)) {
					this->copy_index = this->selected_index;
				}
				if (event.key_pressed(sf::Keyboard::V)) {
					this->paste(event.mouse_position());
				}
				if (event.key_pressed(sf::Keyboard::D)) {
					if (this->selected_index != qpl::size_max) {
						auto before = this->copy_index;
						this->copy_index = this->selected_index;
						this->paste(this->fields[this->selected_index].get_hitbox().get_center());
						this->copy_index = before;
					}
				}
				if (event.key_single_pressed(sf::Keyboard::T)) {
					this->turbo = !this->turbo;
				}
			}
			if (event.key_single_pressed(sf::Keyboard::Backspace)) {
				if (this->selected_index != qpl::size_max) {
					this->draw_order.erase(std::ranges::find(this->draw_order, this->selected_index));
					if (this->selected_index != this->fields.size() - 1) {
						std::swap(this->fields[this->selected_index], this->fields.back());
						for (auto& i : this->draw_order) {
							if (i >= this->selected_index) {
								--i;
							}
						}
					}
					if (this->fields.size()) {
						this->fields.pop_back();
					}
					this->selected_index = qpl::size_max;
				}
			}
		}
		if (event.key_holding(sf::Keyboard::LControl)) {
			if (event.key_single_pressed(sf::Keyboard::S)) {
				this->save();
			}
			if (event.key_single_pressed(sf::Keyboard::L)) {
				this->load();
			}
		}
	}
	void update(const qsf::event_info& event) {
		bool other_selected = false;
		this->any_text_field_focus = false;
		qpl::size just_selected_index = qpl::size_max;

		for (auto it = this->draw_order.crbegin(); it != this->draw_order.crend(); ++it) {
			auto index = *it;

			event.update(this->fields[index], other_selected);
			if (this->fields[index].just_selected) {
				this->selected_index = index;
				just_selected_index = index;
				other_selected = true;
			}
			if (this->fields[index].text_field.has_focus()) {
				any_text_field_focus = true;
			}
		}
		if (just_selected_index != qpl::size_max && just_selected_index != this->fields.size() - 1) {
			this->draw_order.erase(std::ranges::find(this->draw_order, just_selected_index));
			this->draw_order.push_back(just_selected_index);
		}

		this->update_input(event);

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
		for (auto& i : this->draw_order) {
			draw.draw(this->fields[i]);
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