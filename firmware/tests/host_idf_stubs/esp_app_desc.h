#pragma once
struct esp_app_desc_t{char version[32]{};char project_name[32]{};};
extern "C" const esp_app_desc_t* esp_app_get_description();
