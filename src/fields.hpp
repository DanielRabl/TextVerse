#pragma once
#include <qpl/qpl.hpp>
#include "field.hpp"
#include "crypto.hpp"

struct fields {
	std::vector<field> fields;
	std::list<qpl::size> draw_order;

	qpl::size selected_index = qpl::size_max;
	qpl::size copy_index = qpl::size_max;
	bool allow_view_drag = true;
	bool any_text_field_focus = false;
	bool turbo = false;


	void init() {
		this->load();
	}

	void save() {
		qpl::save_state state;

		state.save(crypto::check);
		state.save(this->fields);

		std::vector<qpl::size> order(this->fields.size());
		auto it = this->draw_order.begin();
		for (qpl::size i = 0u; i < this->fields.size(); ++i) {
			order[i] = *it;
			++it;
		}
		state.save(order);

		auto str = qpl::encrypted_keep_size(state.get_finalized_string(), crypto::key);
		qpl::write_data_file(str, "data/session.dat");
	}

	field get_default_field() const {
		field field;
		field.init();
		field.update_background();
		auto hitbox = this->find_free_spot_for(field.get_hitbox());
		field.set_position(hitbox.position);
		return field;
	}
	field get_script_field() const {
		field field;
		field.init();
		field.set_field_type(field_type::execute_script);
		field.update_background();
		auto hitbox = this->find_free_spot_for(field.get_hitbox());
		field.set_position(hitbox.position);
		return field;
	}

	void load_default() {
		this->fields.resize(1u);
		this->fields[0u] = this->get_default_field();
		this->draw_order.push_back(0u);
	}

	void load() {
		if (!qpl::filesys::exists("data/session.dat")) {
			this->load_default();
			return;
		}

		auto data = qpl::filesys::read_file("data/session.dat");
		qpl::decrypt_keep_size(data, crypto::key);

		std::array<qpl::u64, 4u> confirm;

		qpl::save_state state;
		state.set_string(data);

		state.load(confirm);

		if (confirm != crypto::check) {
			qpl::println("couldn't load session!");
			this->load_default();
			return;
		}

		qpl::size size;
		state.load(size);

		this->fields.resize(size);
		for (auto& i : this->fields) {
			i.init();
			state.load(i);
		}

		std::vector<qpl::size> order;
		state.load(order);
		this->draw_order.clear();
		for (auto& i : order) {
			this->draw_order.push_back(i);
		}
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

			bool del = event.key_single_pressed(sf::Keyboard::Backspace) || event.key_single_pressed(sf::Keyboard::Delete);
			if (del) {
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
			if (this->fields[index].text.has_focus()) {
				any_text_field_focus = true;
			}
		}
		if (just_selected_index != qpl::size_max) {
			this->draw_order.erase(std::ranges::find(this->draw_order, just_selected_index));
			this->draw_order.push_back(just_selected_index);
		}

		this->update_input(event);

		bool one_hovering = false;
		this->allow_view_drag = true;
		for (auto& i : this->fields) {
			if (i.text.has_focus() || i.dragging) {
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