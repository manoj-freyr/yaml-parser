/*
 * Example libyaml parser.
 *
 * This is a basic example to demonstrate how to convert yaml to c language
 * data objects using the libyaml emitter API.  For details about libyaml, see
 * the libyaml project page http://pyyaml.org/wiki/LibYAML
 *
 * Example yaml data to be parsed:
 *
 *    $ cat fruit.yaml
 *    ---
 *    fruit:
 *    - name: apple
 *      color: red
 *      count: 12
 *      varieties:
 *      - name: macintosh
 *        seedless: false
 *      - name: granny smith
 *        seedless: false
 *    - name: orange
 *      color: orange
 *      count: 3
 *    ...
 *
 * The sequence of yaml events supported is:
 *
 *    <stream>       ::= STREAM-START <document> STREAM-END
 *    <document>     ::= DOCUMENT-START MAPPING-START "fruit" <fruit-list> MAPPING-END DOCUMENT-END
 *    <fruit-list>   ::= SEQUENCE-START <fruit-obj>* SEQUENCE-END
 *    <fruit-obj>    ::= MAPPING-START <fruit-data>* MAPPING-END
 *    <fruit-data>   ::= "name" <string> |
 *                       "color" <string> |
 *                       "count" <integer> |
 *                       "varieties" <variety-list>
 *    <variety-list> ::= SEQUENCE-START <variety-obj>* SEQUENCE-END
 *    <variety-obj>  ::= MAPPING-START <variety-data>* MAPPING-END
 *    <variety-data> ::= "name" <string> |
 *                       "color" <string> |
 *                       "seedless" (0|1) |
 *
 * For example:
 *
 *    $ cat fruit.yaml | ./scan
 *    stream-start-event (1)
 *      document-start-event (3)
 *        mapping-start-event (9)
 *          scalar-event (6) = {value="fruit", length=5}
 *          sequence-start-event (7)
 *            mapping-start-event (9)
 *              scalar-event (6) = {value="name", length=4}
 *              scalar-event (6) = {value="apple", length=5}
 *              scalar-event (6) = {value="color", length=5}
 *              scalar-event (6) = {value="red", length=3}
 *              scalar-event (6) = {value="count", length=5}
 *              scalar-event (6) = {value="12", length=2}
 *              scalar-event (6) = {value="varieties", length=9}
 *              sequence-start-event (7)
 *                mapping-start-event (9)
 *                  scalar-event (6) = {value="name", length=4}
 *                  scalar-event (6) = {value="macintosh", length=9}
 *                  scalar-event (6) = {value="seedless", length=8}
 *                  scalar-event (6) = {value="false", length=5}
 *                mapping-end-event (10)
 *                mapping-start-event (9)
 *                  scalar-event (6) = {value="name", length=4}
 *                  scalar-event (6) = {value="granny smith", length=12}
 *                  scalar-event (6) = {value="seedless", length=8}
 *                  scalar-event (6) = {value="false", length=5}
 *                mapping-end-event (10)
 *              sequence-end-event (8)
 *            mapping-end-event (10)
 *            mapping-start-event (9)
 *              scalar-event (6) = {value="name", length=4}
 *              scalar-event (6) = {value="orange", length=6}
 *              scalar-event (6) = {value="color", length=5}
 *              scalar-event (6) = {value="orange", length=6}
 *              scalar-event (6) = {value="count", length=5}
 *              scalar-event (6) = {value="3", length=1}
 *            mapping-end-event (10)
 *          sequence-end-event (8)
 *        mapping-end-event (10)
 *      document-end-event (4)
 *    stream-end-event (2)
 *
 */
#include <yaml.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cerrno>
#include <iostream>
#include "gstaction.h"
#include <vector>
/* Set environment variable DEBUG=1 to enable debug output. */
int debug = 0;

/* yaml_* functions return 1 on success and 0 on failure. */
enum status {
    SUCCESS = 1,
    FAILURE = 0
};

/* Our example parser states. */
const std::string ACTIONS{"actions"};
const std::string NAME{"name"};
const std::string PARALLEL{"parallel"};
const std::string DEVICES{"device"};
const std::string MODULENAME{"module"};
const std::string TARGET{"target_stress"};
const std::string COPY_MATRIX{"copy_matrix"};
const std::string MATRIX_SIZE_A{"matrix_size_a"};
const std::string MATRIX_SIZE_B{"matrix_size_b"};
const std::string MATRIX_SIZE_C{"matrix_size_c"};
const std::string COUNT{"count"};
const std::string DURATION{"duration"};
const std::string OPS_TYPE{"ops_type"};
const std::string LOG_INTERVAL{"log_interval"};


enum parse_state {
    STATE_START,    /* start state */
    STATE_STREAM,   /* start/end stream */
    STATE_DOCUMENT, /* start/end document */
    STATE_SECTION,  /* top level */

    STATE_ACTIONLIST,    /* action list */
    STATE_ACTIONVALUES,  /* action key-value pairs */
    STATE_ACTIONKEY,     /* action key */
    STATE_ACTIONNAME,    /* action name value */
    STATE_DEVICES,        /* devices  value */
    STATE_MODULENAME,    /* module name value */
    STATE_PARALLEL,    /* module name value */

    STATE_COUNT,    /* varieties list */
    STATE_DURATION,  /* variety key-value pairs */
    STATE_COPY_MATRIX,    /* variety key */
    STATE_TARGET_STRESS,    /* variety name */
    STATE_SIZE_A,   /* variety color */
    STATE_SIZE_B,   /* variety color */
    STATE_SIZE_C,   /* variety color */
    STATE_OPS,   /* variety color */
    STATE_LOGINTERVAL,   /* variety color */

    STATE_STOP      /* end state */
};

/* Our application parser state data. */
struct parser_state {
    parse_state state;      /* The current parse state */
    gst_action f;        /* Fruit data elements. */
    std::vector<gst_action> actionlist;   /* List of 'fruit' objects. */
};



/*
 * Consume yaml events generated by the libyaml parser to
 * import our data into raw c data structures. Error processing
 * is keep to a mimimum since this is just an example.
 */
int consume_event(struct parser_state *s, yaml_event_t *event)
{
    char *value;
    std::string temp;
		gst_action f;
    if (debug) {
        printf("state=%d event=%d\n", s->state, event->type);
    }
    switch (s->state) {
    case STATE_START:
        switch (event->type) {
        case YAML_STREAM_START_EVENT:
            s->state = STATE_STREAM;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

     case STATE_STREAM:
        switch (event->type) {
        case YAML_DOCUMENT_START_EVENT:
            s->state = STATE_DOCUMENT;
            break;
        case YAML_STREAM_END_EVENT:
            s->state = STATE_STOP;  /* All done. */
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

     case STATE_DOCUMENT:
        switch (event->type) {
        case YAML_MAPPING_START_EVENT:
            s->state = STATE_SECTION;
            break;
        case YAML_DOCUMENT_END_EVENT:
            s->state = STATE_STREAM;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_SECTION:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            value = (char *)event->data.scalar.value;
            if (strcmp(value, ACTIONS.c_str()) == 0) {
               s->state = STATE_ACTIONLIST;
            } else {
               fprintf(stderr, "Unexpected scalar: %s\n", value);
               return FAILURE;
            }
            break;
        case YAML_DOCUMENT_END_EVENT:
            s->state = STATE_STREAM;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_ACTIONLIST:
        switch (event->type) {
        case YAML_SEQUENCE_START_EVENT:
            s->state = STATE_ACTIONVALUES;
            break;
        case YAML_MAPPING_END_EVENT:
            s->state = STATE_SECTION;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_ACTIONVALUES:
        switch (event->type) {
        case YAML_MAPPING_START_EVENT:
            s->state = STATE_ACTIONKEY;
            break;
        case YAML_SEQUENCE_END_EVENT:
            s->state = STATE_ACTIONLIST;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_ACTIONKEY:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            value = (char *)event->data.scalar.value;
            if (strcmp(value, NAME.c_str()) == 0) {
                s->state = STATE_ACTIONNAME;
            } else if (strcmp(value, PARALLEL.c_str()) == 0) {
                s->state = STATE_PARALLEL;
            } else if (strcmp(value, COUNT.c_str()) == 0) {
                s->state = STATE_COUNT;
            } else if (strcmp(value, TARGET.c_str())== 0) {
                s->state = STATE_TARGET_STRESS;
            } else if (strcmp(value, COPY_MATRIX.c_str())== 0) {
                s->state = STATE_COPY_MATRIX;
            } else if (strcmp(value, MATRIX_SIZE_A.c_str())== 0) {
                s->state = STATE_SIZE_A;
            } else if (strcmp(value, MATRIX_SIZE_B.c_str())== 0) {
                s->state = STATE_SIZE_B;
            } else if (strcmp(value, MATRIX_SIZE_C.c_str())== 0) {
                s->state = STATE_SIZE_C;
            } else if (strcmp(value, OPS_TYPE.c_str())== 0) {
                s->state = STATE_OPS;
            } else if (strcmp(value, DEVICES.c_str())== 0) {
                s->state = STATE_DEVICES;
            } else if (strcmp(value, MODULENAME.c_str())== 0) {
                s->state = STATE_MODULENAME;
            } else if (strcmp(value, DURATION.c_str())== 0) {
                s->state = STATE_DURATION;
            } else if (strcmp(value, LOG_INTERVAL.c_str())== 0) {
                s->state = STATE_LOGINTERVAL;
            } else {
                fprintf(stderr, "Unexpected key: %s\n", value);
                return FAILURE;
            }
            break;

        case YAML_MAPPING_END_EVENT:
            add_action(s->actionlist, s->f.m_name, s->f.m_module_name,s->f.m_count, s->f.m_ops,
									s->f.m_target_stress, s->f.m_duration, s->f.m_size_a,
									s->f.m_size_b, s->f.m_size_c, s->f.m_log_interval, 
									s->f.m_parallel, s->f.m_copy_matrix);
            s->state = STATE_ACTIONVALUES;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_ACTIONNAME:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            s->f.m_name = std::string{(char *)event->data.scalar.value};
						std::cout << "Name is " << s->f.m_name << std::endl;
            s->state = STATE_ACTIONKEY;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_MODULENAME:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
						temp = (char *)event->data.scalar.value;
						std::cout << "MANOJ:" << temp << std::endl;
            s->f.m_module_name = temp;
						temp.clear();
						std::cout << "MANOJ:" << s->f.m_module_name << std::endl;
            s->state = STATE_ACTIONKEY;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_DEVICES:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
						s->f.m_devices = "okay";
						std::cout << (char *)event->data.scalar.value << std::endl;
						std::cout << " and" << s->f.m_devices << "ok" << std::endl;
            s->f.m_devices = (char *)event->data.scalar.value;
            s->state = STATE_ACTIONKEY;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_COUNT:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            s->f.m_count = atoi((char *)event->data.scalar.value);
            s->state = STATE_ACTIONKEY;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_DURATION:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            s->f.m_duration = atoi((char *)event->data.scalar.value);
            s->state = STATE_ACTIONKEY;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_COPY_MATRIX:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
						temp = std::string{(char *)event->data.scalar.value};
            s->f.m_copy_matrix = (temp == "true") ? true : false;
            s->state = STATE_ACTIONKEY;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_PARALLEL:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            temp = std::string{(char *)event->data.scalar.value};
            s->f.m_parallel = (temp == "true") ? true : false;
            s->state = STATE_ACTIONKEY;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_SIZE_A:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            s->f.m_size_a = atoi((char *)event->data.scalar.value);
            s->state = STATE_ACTIONKEY;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_SIZE_B:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            s->f.m_size_b = atoi((char *)event->data.scalar.value);
            s->state = STATE_ACTIONKEY;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_SIZE_C:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            s->f.m_size_c = atoi((char *)event->data.scalar.value);
            s->state = STATE_ACTIONKEY;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_LOGINTERVAL:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            s->f.m_log_interval = atoi((char *)event->data.scalar.value);
            s->state = STATE_ACTIONKEY;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_TARGET_STRESS:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            s->f.m_target_stress = atof((char *)event->data.scalar.value);
            s->state = STATE_ACTIONKEY;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_OPS:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            s->f.m_ops = std::string{(char *)event->data.scalar.value};
            s->state = STATE_ACTIONKEY;
            break;
        default:
            fprintf(stderr, "Unexpected event %d in state %d.\n", event->type, s->state);
            return FAILURE;
        }
        break;

    case STATE_STOP:
        break;
    }
    return SUCCESS;
}

int
main(int argc, char *argv[])
{
    int code;
    int status_;
    struct parser_state state;
    yaml_parser_t parser;
    yaml_event_t event;

    if (getenv("DEBUG")) {
        debug = 1;
    }
		FILE *fd = fopen("/home/amd/freyr/exper/yaml-tools/libyaml/gst_single.conf", "r");
		if(!fd){
			std::cout << "Could not open file " << std::endl;
			return -1;
		}
    memset(&state, 0, sizeof(state));
    state.state = STATE_START;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, fd);
    do {
        status_ = yaml_parser_parse(&parser, &event);
        if (status_ == FAILURE) {
            fprintf(stderr, "yaml_parser_parse error\n");
            code = EXIT_FAILURE;
            goto done;
        }
        status_ = consume_event(&state, &event);
        if (status_ == FAILURE) {
            fprintf(stderr, "consume_event error\n");
            code = EXIT_FAILURE;
            goto done;
        }
        yaml_event_delete(&event);
    } while (state.state != STATE_STOP);

    /* Output the parsed data. */
    for (auto f : state.actionlist ) {
        printf("action: name=%s,count=%d, modname=%s\n", f.m_name.c_str(),
						f.m_count, f.m_module_name.c_str());
    }
    code = EXIT_SUCCESS;

done:
    destroy_actions(state.actionlist);
    yaml_parser_delete(&parser);
    return code;
}
