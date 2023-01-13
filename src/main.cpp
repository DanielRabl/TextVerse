#include <qpl/qpl.hpp>
#include "widgets.hpp"

struct main_state : qsf::base_state {
	void init() override {
		this->clear_color = qpl::rgb::grey_shade(20);

		this->widgets.init();
		this->call_on_resize();

		this->view.set_position(this->widgets.view_position);
		this->view.set_scale(this->widgets.view_scale);
	}
	void call_on_resize() override {
		this->view.set_hitbox(*this);
	}
	void call_on_close() override {
		this->widgets.save();
	}
	void updating() override {
		this->update(this->view);
		if (this->view.just_changed()) {
			this->widgets.view_position = this->view.position;
			this->widgets.view_scale = this->view.scale;
		}

		if (this->event().key_holding(sf::Keyboard::LControl) && this->event().key_pressed(sf::Keyboard::R)) {
			this->view.reset();
			this->view.set_hitbox(*this);
		}

		this->update(this->widgets, this->view);

		this->view.allow_dragging = this->widgets.allow_view_drag;
	}
	void drawing() override {
		this->draw(this->widgets, this->view);
	}
	qsf::view_rectangle view;
	qpl::size side = 0u;
	widgets widgets;
};

int main() try {
	qsf::framework framework;
	framework.set_antialiasing_level(2);
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