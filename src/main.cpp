#include <qpl/qpl.hpp>
#include "widgets.hpp"

struct main_state : qsf::base_state {
	void init() override {
		this->clear_color = qpl::rgb::grey_shade(20);

		this->load();
		this->call_on_resize();

		this->color_picker.set_font("helvetica");
		this->color_picker.view.set_position({ 200, 0 });
	}
	void call_on_resize() override {
		this->view.set_hitbox(*this);
	}
	void call_on_close() override {
		if (this->save_on_close) {
			this->save();
		}
	}

	void save() {
		qpl::save_state state;
		state.save(this->widgets, this->view.position, this->view.scale, crypto::check);
	
		auto str = qpl::encrypted_keep_size(state.get_finalized_string(), crypto::key);
		qpl::write_data_file(str, "data/session.dat");
	}
	void load() {
		auto data = qpl::filesys::read_file("data/session.dat");
		qpl::decrypt_keep_size(data, crypto::key);
	
		std::array<qpl::u64, 4u> confirm;
		qpl::load_state state;
		state.set_string(data);
		state.load(this->widgets, this->view.position, this->view.scale, confirm);
	
		if (confirm != crypto::check) {
			qpl::println("couldn't load session!");
			this->widgets.load_default();
			return;
		}
	}

	void updating() override {
		this->update(this->color_picker, this->view);

		if (this->event().key_holding(sf::Keyboard::LControl)) {
			if (this->event().key_pressed(sf::Keyboard::R)) {
				this->view.reset();
					this->view.set_hitbox(*this);
			}
			else if (this->event().key_single_pressed(sf::Keyboard::S)) {
				this->save();
			}
			else if (this->event().key_single_pressed(sf::Keyboard::L)) {
				this->load();
			}
		}
		if (this->event().key_holding(sf::Keyboard::Space)) {
			this->color_picker.set_color_value(this->color_picker.get_color_value());
		}

		this->update(this->widgets, this->view);
		this->view.allow_dragging = this->widgets.allow_view_drag && !this->color_picker.has_focus();
		this->update(this->view);
	}

	void drawing() override {
		this->draw(this->widgets, this->view);
		this->draw(this->color_picker, this->view);
	}
	qsf::view_control view;
	qpl::size side = 0u;
	widgets widgets;
	bool save_on_close = false;

	qsf::view_extension<qsf::color_picker> color_picker;
};

int main() try {

	qsf::framework framework;
	framework.set_antialiasing_level(1);
	framework.set_title("QPL");
	framework.add_font("helvetica", "resources/Helvetica.ttf");
	framework.add_font("consola", "resources/consola.ttf");
	framework.set_dimension({ 1400u, 950u });

	framework.add_state<main_state>();
	framework.game_loop();
}
catch (std::exception& any) {
	qpl::println("caught exception:\n", any.what());
	qpl::system_pause();
}