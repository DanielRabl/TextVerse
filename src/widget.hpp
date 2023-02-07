#pragma once
#include <qpl/qpl.hpp>
#include "executable_script.hpp"

enum class widget_type {
	text,
	executable_script,
};

struct widget {
	qsf::view view;
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
		return this->view.transform_hitbox(this->hitbox);
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
		this->view = other.view;
		this->hovering = false;
		this->dragging = false;
		this->just_selected = false;

		if (other.executable_script) {
			this->executable_script = std::make_unique<::executable_script>(*other.executable_script);
		}
		else {
			this->executable_script.reset();
		}
		return *this;
	}

	void save(qpl::save_state& state) const {
		state.save(this->text.wstring());
		state.save(this->view.position);
		state.save(this->view.scale);
		state.save(this->type);
	}
	bool load(qpl::load_state& state) {
		this->init();

		std::wstring text_string;
		state.load(text_string);

		state.load(this->view.position);
		state.load(this->view.scale);

		state.load(this->type);

		this->set_widget_type(this->type);
		this->text.set_string(text_string);
		return true;
	}
	void init() {
		this->text.set_font("helvetica");
		this->text.set_text_character_size(40);
		this->text.background_increase = { 20, 20 };
		this->text.background.set_outline_thickness(5.0f);
		this->text.background.set_outline_color(qpl::rgb::black());
		this->text.background.set_slope_dimension(config::widget_slope_dimension);
		this->text.set_position({ 40, 70 });

		this->background.set_color(this->background_color);
		this->background.set_slope_dimension(config::widget_background_slope_dimension);
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
		auto delta = position - this->view.position;
		this->move(delta);
	}
	void move(qpl::vec2 delta) {
		this->view.move(delta);
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

	void update_execute_script(const qsf::event_info& event) {

		if (this->executable_script) {
			event.update(*this->executable_script);
			if (this->executable_script->clicked) {
				auto lines = qpl::string_split(this->text.string(), '\n');

				std::unordered_map<std::string, std::string> variables;

				auto get_word_with_variables = [&](std::string word) {
					std::string result = "";
					qpl::size begin = qpl::size_max;
					qpl::size end = 0u;
					for (qpl::size i = 0u; i < word.length(); ++i) {
						if (word[i] == '$') {
							if (begin == qpl::size_max) {
								result += word.substr(end, i - end);
								begin = i + 1;
							}
							else {
								auto name = word.substr(begin, i - begin);
								if (variables.find(name) != variables.cend()) {
									auto value = variables[name];
									result += value;
								}
								end = i + 1;
								begin = qpl::size_max;
							}
						}
					}
					result += word.substr(end, word.length() - end);
					return result;
				};

				for (auto& line : lines) {
					auto words = qpl::string_split(line);
					if (!words.empty()) {
						auto command = words[0];

						if (!command.empty()) {
							if (qpl::string_equals_ignore_case(command, "copy")) {
								if (words.size() == 3u) {

									auto src = get_word_with_variables(words[1]);
									auto dest = get_word_with_variables(words[2]);

									qpl::println("copy ", qpl::foreground::aqua, src, "to ", qpl::foreground::aqua, dest);
									qpl::filesys::copy_overwrite(src, dest);
								}
								else {
									qpl::println("copy: invalid number of arguments.");
								}
							}
							else if (qpl::string_equals_ignore_case(command, "move")) {
								if (words.size() == 3u) {
									auto src = get_word_with_variables(words[1]);
									auto dest = get_word_with_variables(words[2]);

									qpl::println("move ", qpl::foreground::aqua, src, "to ", qpl::foreground::aqua, dest);
									qpl::filesys::move_overwrite(src, dest);
								}
								else {
									qpl::println("move: invalid number of arguments.");
								}
							}
							else if (qpl::string_equals_ignore_case(command, "remove")) {
								if (words.size() == 2u) {
									auto src = get_word_with_variables(words[1]);
									qpl::println("remove ", qpl::foreground::aqua, src);
									qpl::filesys::remove(src);
								}
								else {
									qpl::println("remove: invalid number of arguments.");
								}
							}
							else if (qpl::string_equals_ignore_case(command, "rename")) {
								if (words.size() == 3u) {
									qpl::println("rename ", qpl::foreground::aqua, words[1], " to ", qpl::foreground::aqua, words[2]);
									qpl::filesys::rename(words[1], words[2]);
								}
								else {
									qpl::println("rename: invalid number of arguments.");
								}
							}
							else if (qpl::string_equals_ignore_case(command, "sync")) {
								if (words.size() == 3u) {

									qpl::filesys::path src = get_word_with_variables(words[1]);
									qpl::filesys::path dest = get_word_with_variables(words[2]);

									if (!src.exists() && !dest.exists()) {
										qpl::println("sync: both paths don't exist.");
									}
									else if (!src.exists()) {
										src.create();
										qpl::println("sync ", qpl::foreground::aqua, dest, " to ", qpl::foreground::aqua, src);
										qpl::filesys::copy_overwrite(dest, src);
									}
									else if (!dest.exists()) {
										dest.create();
										qpl::println("sync ", qpl::foreground::aqua, src, " to ", qpl::foreground::aqua, dest);
										qpl::filesys::copy_overwrite(src, dest);
									}
									else {
										auto a_time = src.last_write_time();
										auto b_time = dest.last_write_time();

										if (a_time < b_time) {
											qpl::println("sync ", qpl::foreground::aqua, dest, " to ", qpl::foreground::aqua, src);
											qpl::filesys::copy_overwrite(dest, src);
										}
										else if (b_time < a_time) {
											qpl::println("sync ", qpl::foreground::aqua, src, " to ", qpl::foreground::aqua, dest);
											qpl::filesys::copy_overwrite(src, dest);
										}
										else {
											qpl::println(qpl::foreground::aqua, src, " and ", qpl::foreground::aqua, dest, " are synchronized already.");
										}
									}
								}
								else {
									qpl::println("sync: invalid number of arguments.");
								}
							}
							else if (command[0] == '$' && qpl::count(command, '$') == 1u) {
								auto name = command.substr(1u);
								std::string value = "";
								for (qpl::size i = 1u; i < words.size(); ++i) {
									bool equals_sign = (words[i] == "=" || words[i] == ":");
									if (!equals_sign) {
										value = get_word_with_variables(words[i]);
										break;
									}
								}
								if (!value.empty()) {
									variables[name] = value;
								}
							}
							else {
								qpl::print("ignored command: \"");
								for (qpl::size i = 0u; i < words.size(); ++i) {
									if (i) {
										qpl::print(' ');
									}
									qpl::print(get_word_with_variables(words[i]));
								}
								qpl::println("\"");
							}
						}
					}
				}
			}
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

		this->update_execute_script(event);
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