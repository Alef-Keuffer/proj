#include <vector>
#include <stdio.h>
#include <string.h>
#include "tinyxml2.h"

/*I think this is usually done with macros, but I want to know why not do what I'm doing*/
static const int TRANSLATE = 1;
static const int ROTATE = 2;
static const int SCALE = 3;
static const int LOAD_MODEL = 4;
static const int BEGIN_GROUP = 5;
static const int END_GROUP = 6;

/*! @addtogroup Operations
 * @{
 * # Data structure for Operations
 *
 * The `elem` will be
 *
 * -# `⟨TRANSFORM⟩ ⟨float⟩ ⟨float⟩ ⟨float⟩ [float]` or
 * -# `⟨LOAD_MODEL⟩ ⟨NUMBER_OF_CHARACTERS⟩ ⟨char⟩ ... ⟨char⟩`
 *
 * and `groups` will be
 *
 * - `⟨BEGIN_GROUP⟩⟨BEGIN_GROUP⟩⃰  ⟨elem⟩⁺ ⟨END_GROUP⟩⃰ ⟨END_GROUP⟩`
 *
 * for example (inside a group),
 *
 * @code{.unparsed}LOAD_MODEL 7 'e' 'x' 'a' 'm' 'p' 'l' 'e'@endcode
 *
 * our operations (`world`) will be
 *
 * - `⟨position⟩⟨lookAt⟩⟨up⟩⟨projection⟩⟨group⟩⁺
 *
 * where `position`, `lookAt`, `up` and ⟨projection⟩ are
 *
 * - ⟨float⟩⟨float⟩⟨float⟩
 *
 * we'll use an array of floats, therefore we will need to cast to char when reading
 * the model filename characters and to int when reading the number of characters.
 */

/*! @addtogroup Transforms
 * @{*/
void operations_push_transform_attributes(tinyxml2::XMLElement *transform, std::vector<float> *operations) {
    if (!transform) return;

    float angle = transform->FloatAttribute("angle");
    if ((int) angle)
        operations->push_back(angle);

    operations->push_back(transform->FloatAttribute("x"));
    operations->push_back(transform->FloatAttribute("y"));
    operations->push_back(transform->FloatAttribute("z"));
}

void operations_push_transforms(tinyxml2::XMLElement *transforms, std::vector<float> *operations) {
    tinyxml2::XMLElement *transform = transforms->FirstChildElement();

    do {
        const char *transformValue = transform->Value();

        if (!strcmp("translate", transformValue))
            operations->push_back(TRANSLATE);
        else if (!strcmp("rotate", transformValue))
            operations->push_back(ROTATE);
        else if (!strcmp("scale", transformValue))
            operations->push_back(SCALE);
        else {
            fprintf(stderr, "Unknown transform: \"%s\"", transformValue);
            exit(1);
        }

        operations_push_transform_attributes(transform, operations);

    } while ((transform = transform->NextSiblingElement()));
}
//! @} end of group Transforms

/*! @addtogroup Models
 * @{
 */

void operations_push_model(tinyxml2::XMLElement *model, std::vector<float> *operations) {
    unsigned int i = 0;
    const char *string = model->Attribute("file");

    operations->push_back(LOAD_MODEL);

    do operations->push_back(string[i++]);
    while (string[i]);

    if (i <= 0) {
        fprintf(stderr, "filename is empty");
        exit(1);
    }

    //i--;

    operations->insert(operations->end() - i, (float) i);
}

void operations_push_models(tinyxml2::XMLElement *models, std::vector<float> *operations) {
    tinyxml2::XMLElement *model = models->FirstChildElement("model");
    do operations_push_model(model, operations);
    while ((model = model->NextSiblingElement("model")));
}
//! @} end of group Models

/*! @addtogroup Groups
 * @{*/
void operations_push_groups(tinyxml2::XMLElement *group, std::vector<float> *operations) {
    operations->push_back(BEGIN_GROUP);

    tinyxml2::XMLElement *transforms = group->FirstChildElement("transform");
    tinyxml2::XMLElement *models = group->FirstChildElement("models");

    if (transforms) operations_push_transforms(transforms, operations);
    if (models) operations_push_models(models, operations);

    tinyxml2::XMLElement *childGroup = group->FirstChildElement("group");
    if (childGroup)
        do operations_push_groups(childGroup, operations);
        while ((childGroup = childGroup->NextSiblingElement("group")));

    operations->push_back(END_GROUP);
}
//! @} end of group Groups

/*! @addtogroup xml
 *@{*/

void operations_load_xml(const char *filename, std::vector<float> *operations) {
    tinyxml2::XMLDocument doc;

    if (doc.LoadFile(filename)) {
        if (doc.ErrorID() == tinyxml2::XML_ERROR_FILE_NOT_FOUND)
          fprintf(stderr, "Failed loading file: '%s'\n", filename);
        fprintf(stderr, "%s", doc.ErrorName());
        exit(1);
    }

    fprintf(stderr, "Loaded file: '%s'\n", filename);
    tinyxml2::XMLElement *world = doc.FirstChildElement("world");

    /*camera*/
    tinyxml2::XMLElement *camera = world->FirstChildElement("camera");

    tinyxml2::XMLElement *position = camera->FirstChildElement("position");
    operations_push_transform_attributes(position, operations);

    tinyxml2::XMLElement *lookAt = camera->FirstChildElement("lookAt");
    operations_push_transform_attributes(lookAt, operations);

    tinyxml2::XMLElement *up = camera->FirstChildElement("up");
    if (up) operations_push_transform_attributes(up, operations);
    else operations->insert(operations->end(), {0, 1, 0});

    tinyxml2::XMLElement *projection = camera->FirstChildElement("projection");
    if (projection) {
        operations->push_back(projection->FloatAttribute("fov"));
        operations->push_back(projection->FloatAttribute("near"));
        operations->push_back(projection->FloatAttribute("far"));
    } else operations->insert(operations->end(), {60, 1, 1000});
    /*end of camera*/

    // groups
    tinyxml2::XMLElement *group = world->FirstChildElement("group");
    operations_push_groups(group, operations);
}
//! @} end of group xml

/*! @addtogroup Printing
 * @{*/
void operations_print(std::vector<float> *operations) {
    unsigned int i = 0;
    fprintf(stderr,
            "POSITION(%.2f %.2f %.2f)\n",
            operations->at(i),
            operations->at(i + 1),
            operations->at(i + 2));
    i += 3;
    fprintf(stderr,
            "LOOK_AT(%.2f %.2f %.2f)\n",
            operations->at(i),
            operations->at(i + 1),
            operations->at(i + 2));
    i += 3;
    fprintf(stderr,
            "UP(%.2f %.2f %.2f)\n",
            operations->at(i),
            operations->at(i + 1),
            operations->at(i + 2));
    i += 3;
    fprintf(stderr,
            "PROJECTION(%.2f %.2f %.2f)\n",
            operations->at(i),
            operations->at(i + 1),
            operations->at(i + 2));

    for (i += 3; i < operations->size(); i++) {
        switch ((int) operations->at(i)) {
            case ROTATE:
                fprintf(stderr,
                        "ROTATE(%.2f %.2f %.2f %.2f)\n",
                        operations->at(i + 1),
                        operations->at(i + 2),
                        operations->at(i + 3),
                        operations->at(i + 4));
                i += 4;
                continue;
            case TRANSLATE:
                fprintf(stderr,
                        "TRANSLATE(%.2f %.2f %.2f)\n",
                        operations->at(i + 1),
                        operations->at(i + 2),
                        operations->at(i + 3));
                i += 3;
                continue;
            case SCALE:
                fprintf(stderr,
                        "SCALE(%.2f %.2f %.2f)\n",
                        operations->at(i + 1),
                        operations->at(i + 2),
                        operations->at(i + 3));
                i += 3;
                continue;
            case BEGIN_GROUP:
                fprintf(stderr, "BEGIN_GROUP\n");
                continue;
            case END_GROUP:
                fprintf(stderr, "END_GROUP\n");
                continue;
            case LOAD_MODEL:
                int stringSize = (int) operations->at(i + 1);
                char model[stringSize + 1];

                int j;
                for (j = 0; j < stringSize; j++)
                    model[j] = (char) operations->at(i + 2 + j);

                model[j] = 0;

                fprintf(stderr, "LOAD_MODEL(%s)\n", model);
                i += 1 + j - 1; //just to be explicit
                continue;
        }
    }
}
//! @} end of group Printing
//! @} end of group Operations




void example1(const char *filename) {
    tinyxml2::XMLDocument doc;

    if (doc.LoadFile(filename)) {
        fprintf(stderr, "%s", doc.ErrorName());
        exit(1);
    }

    std::vector<float> operations;
    operations_load_xml(filename, &operations);
    operations_print(&operations);
}

int main2(int argc, char *argv[]) {
    example1("test_files_phase_2/test_2_3.xml");
    return 1;
}