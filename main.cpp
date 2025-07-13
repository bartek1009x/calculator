#include <stdio.h>
#include <cstdlib>
#include <string>
#include <cmath>
#include <vector>
#include <stdexcept>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#define NUM_WINDOWS 15

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    char text;
    int number;
    TTF_Font* font;
} WindowData;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    std::string result;
    int number;
    TTF_Font* font;
} ResultWindowData;

static WindowData windows[NUM_WINDOWS];
static ResultWindowData result_window;
static std::string operation = "0";
static std::string operationResult = "";

static double evaluateExpression(const std::string& expr) {
    std::vector<double> numbers;
    std::vector<char> operators;
    std::string num;

    // Parse the expression into numbers and operators
    for (size_t i = 0; i <= expr.length(); ++i) {
        if (i == expr.length() || expr[i] == '+' || expr[i] == '-' ||
            expr[i] == '*' || expr[i] == '/') {
            if (!num.empty()) {
                numbers.push_back(std::stod(num));
                num.clear();
            }
            if (i < expr.length()) {
                operators.push_back(expr[i]);
            }
        }
        else {
            num += expr[i];
        }
    }

    // First pass: handle * and /
    for (size_t i = 0; i < operators.size();) {
        if (operators[i] == '*' || operators[i] == '/') {
            double left = numbers[i];
            double right = numbers[i + 1];
            double result;

            if (operators[i] == '*') {
                result = left * right;
            }
            else { // '/'
                if (right == 0) throw std::runtime_error("Division by zero");
                result = left / right;
            }

            // Replace the two numbers with their result
            numbers[i] = result;
            numbers.erase(numbers.begin() + i + 1);
            operators.erase(operators.begin() + i);
        }
        else {
            ++i;
        }
    }

    // Second pass: handle + and -
    double result = numbers[0];
    for (size_t i = 0; i < operators.size(); ++i) {
        if (operators[i] == '+') {
            result += numbers[i + 1];
        }
        else { // '-'
            result -= numbers[i + 1];
        }
    }

    return result;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    SDL_SetAppMetadata("Example Multiple Windows with Numbers", "1.0", "com.example.multiple-windows");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (TTF_Init() == -1) {
        return SDL_APP_FAILURE;
    }

    for (int i = 0; i < NUM_WINDOWS; i++) {
        char window_title[32];
        SDL_snprintf(window_title, sizeof(window_title), "Window %d", i + 1);

        if (!SDL_CreateWindowAndRenderer(window_title, 200, 200, 0, &windows[i].window, &windows[i].renderer)) {
            SDL_Log("Couldn't create window/renderer %d: %s", i, SDL_GetError());
            return SDL_APP_FAILURE;
        }

        windows[i].number = i;
        if (i == 14) {
            windows[i].text = '/';
        } else if (i == 13) {
            windows[i].text = '*';
        } else if (i == 12) {
            windows[i].text = '=';
        } else if (i == 11) {
            windows[i].text = '-';
        } else if (i == 10) {
            windows[i].text = '+';
        } else {
            windows[i].text = '0' + i;
        }

        windows[i].font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 64);
        if (!windows[i].font) {
            return SDL_APP_FAILURE;
        }

		SDL_SetWindowPosition(windows[i].window, rand() % 1800, rand() % 900);
    }

    char window_title[32];
    SDL_snprintf(window_title, sizeof(window_title), "Result");

    if (!SDL_CreateWindowAndRenderer(window_title, 860, 200, 0, &result_window.window, &result_window.renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    result_window.result = "0";
    result_window.font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 64);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {

        for (WindowData window : windows) {
            if (SDL_GetWindowFlags(window.window) & SDL_WINDOW_MOUSE_FOCUS) {
                if (window.text == '=') {
                    double evaluation = evaluateExpression(operation);
                    operationResult = std::to_string(evaluation);
                    if (std::floor(evaluation) == evaluation) {
                        operationResult.erase(operationResult.find_last_not_of('0') + 1);
                        operationResult.erase(operationResult.find_last_not_of('.') + 1);
                    }
                } else {
                    if (operation == "0") {operation = window.text; return SDL_APP_CONTINUE;}
                    if (!operationResult.empty()) {
                        operationResult = "";
                        operation = "";
                    }
                    operation += window.text;
                }

                break;
            }
        }
    } else if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    for (int i = 0; i < NUM_WINDOWS; i++) {
        SDL_SetRenderDrawColor(windows[i].renderer, 255, 255, 255, 255);
        SDL_RenderClear(windows[i].renderer);

        char num_str[1] = { windows[i].text };

        SDL_Color black = { 0, 0, 0, 255 };
        SDL_Surface* surface = TTF_RenderText_Solid(windows[i].font, num_str, sizeof(num_str), black);
        if (!surface) {
            continue;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(windows[i].renderer, surface);
        if (!texture) {
            SDL_Log("Couldn't create texture for window %d: %s", i, SDL_GetError());
            SDL_DestroySurface(surface);
            continue;
        }

        int texW = surface->w;
        int texH = surface->h;
        SDL_DestroySurface(surface);
        SDL_FRect dstrect = { (200.0f - texW) / 2.0f, (200.0f - texH) / 2.0f, (float)texW, (float)texH };
        SDL_RenderTexture(windows[i].renderer, texture, NULL, &dstrect);
        SDL_DestroyTexture(texture);

        SDL_RenderPresent(windows[i].renderer);
    }

    SDL_SetRenderDrawColor(result_window.renderer, 255, 255, 255, 255);
    SDL_RenderClear(result_window.renderer);

    if (operationResult.empty()) {
        SDL_Log(operation.c_str());
        result_window.result = operation;
    } else {
        result_window.result = operationResult.c_str();
    }

    SDL_Color black = { 0, 0, 0, 255 };
    SDL_Surface* surface = TTF_RenderText_Solid(result_window.font, result_window.result.c_str(), result_window.result.length(), black);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(result_window.renderer, surface);

    int texW = surface->w;
    int texH = surface->h;
    SDL_DestroySurface(surface);
    SDL_FRect dstrect = { (850 - texW) / 2.0f, (200.0f - texH) / 2.0f, (float)texW, (float)texH };
    SDL_RenderTexture(result_window.renderer, texture, NULL, &dstrect);
    SDL_DestroyTexture(texture);

    SDL_RenderPresent(result_window.renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    for (int i = 0; i < NUM_WINDOWS; i++) {
        if (windows[i].font) {
            TTF_CloseFont(windows[i].font);
        }
    }
    TTF_Quit();
}
