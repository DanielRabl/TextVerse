#pragma once
#include <qpl/qpl.hpp>
#include "widget.hpp"
#include "crypto.hpp"

struct widgets {
	std::vector<widget> widgets;
	std::list<qpl::size> draw_order;

	qpl::size selected_index = qpl::size_max;
	qpl::size copy_index = qpl::size_max;
	bool allow_view_drag = true;
	bool any_text_field_focus = false;
	bool turbo = false;

	qpl::vec2 view_position;
	qpl::vec2 view_scale;

	void init() {
		this->load();
	}

	void save() {
		qpl::save_state state;

		state.save(crypto::check);
		state.save(this->widgets);

		std::vector<qpl::size> order(this->widgets.size());
		auto it = this->draw_order.begin();
		for (qpl::size i = 0u; i < this->widgets.size(); ++i) {
			order[i] = *it;
			++it;
		}
		state.save(order);
		state.save(this->view_position);
		state.save(this->view_scale);

		auto str = qpl::encrypted_keep_size(state.get_finalized_string(), crypto::key);
		qpl::write_data_file(str, "data/session.dat");
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

		this->widgets.resize(size);
		for (auto& i : this->widgets) {
			i.init();
			state.load(i);
		}

		std::vector<qpl::size> order;
		state.load(order);
		this->draw_order.clear();
		for (auto& i : order) {
			this->draw_order.push_back(i);
		}
		state.load(this->view_position);
		state.load(this->view_scale);
	}


	widget get_default_widget() const {
		widget widget;
		widget.init();
		widget.update_background();
		auto hitbox = this->find_free_spot_for(widget.get_hitbox());
		widget.set_position(hitbox.position);
		return widget;
	}
	widget get_executable_script() const {
		widget widget;
		widget.init();
		widget.set_widget_type(widget_type::executable_script);
		widget.update_background();
		auto hitbox = this->find_free_spot_for(widget.get_hitbox());
		widget.set_position(hitbox.position);
		return widget;
	}

	void load_default() {
		this->widgets.resize(1u);
		this->widgets[0u] = this->get_default_widget();
		this->draw_order.push_back(0u);
	}

	bool hitbox_collides_with_widget(qpl::hitbox hitbox) const {
		for (auto& i : this->widgets) {
			if (hitbox.collides(i.get_hitbox())) {
				return true;
			}
		}
		return false;
	}

	qpl::hitbox find_free_spot_for(qpl::hitbox hitbox) const {

		std::vector<std::pair<qpl::size, qpl::f64>> distances(this->widgets.size());
		for (qpl::size i = 0u; i < this->widgets.size(); ++i) {
			distances[i].first = i;
			distances[i].second = (hitbox.get_center() - this->widgets[i].get_hitbox().get_center()).length();
		}
		qpl::sort(distances, [](auto a, auto b) {
			return a.second < b.second;
			});


		std::array<qpl::size, 4u> sides;
		for (qpl::size i = 0u; i < sides.size(); ++i) {
			sides[i] = i;
		}

		for (auto& widget : distances) {
			auto widget_hitbox = this->widgets[widget.first].get_hitbox();
			qpl::shuffle(sides);

			for (const auto& i : sides) {
				auto pos = widget_hitbox.get_side_corner_left((i + 2) % 4);
				pos += qpl::vec_cross4[(i + 1) % 4] * 10;

				hitbox.set_side_corner_left(i, pos);
				if (!this->hitbox_collides_with_widget(hitbox)) {
					return hitbox;
				}
			}
		}
		throw qpl::exception("widgets::find_free_spot_for(", hitbox.string(), ") : no spot found. this shouldn't be possible!");
		return {};
	}

	void paste(qpl::vec2f position) {
		if (this->copy_index != qpl::size_max) {
			auto hitbox = this->widgets[this->copy_index].get_hitbox();
			hitbox.set_center(position);

			if (this->hitbox_collides_with_widget(hitbox)) {
				hitbox = this->find_free_spot_for(hitbox);
			}

			this->widgets.push_back(this->widgets[this->copy_index]);
			this->widgets.back().set_position(hitbox.position);
			this->widgets.back().update_background();
			this->draw_order.push_back(this->widgets.size() - 1);
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
						this->paste(this->widgets[this->selected_index].get_hitbox().get_center());
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
					if (this->selected_index != this->widgets.size() - 1) {
						std::swap(this->widgets[this->selected_index], this->widgets.back());
						for (auto& i : this->draw_order) {
							if (i >= this->selected_index) {
								--i;
							}
						}
					}
					if (this->widgets.size()) {
						this->widgets.pop_back();
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

			event.update(this->widgets[index], other_selected);
			if (this->widgets[index].just_selected) {
				this->selected_index = index;
				just_selected_index = index;
				other_selected = true;
			}
			if (this->widgets[index].text.has_focus()) {
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
		for (auto& i : this->widgets) {
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
			draw.draw(this->widgets[i]);
		}
	}
};