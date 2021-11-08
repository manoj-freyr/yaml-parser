#include <cstdlib>
#include "gstaction.h"

void add_action(gst_action *&actions, std::string name,  int count,
                  std::string ops, float target_stress, int duration,
                  int size_a, int size_b, int size_c,  int log_interval,
                  bool parallel, bool copy_matrix) {
    /* Create fruit object. */
    gst_action *f = new gst_action{}; 
    f->m_name = name;
    f->m_count = count;
		f->m_ops = ops;
		f->m_target_stress = target_stress;
		f->m_duration = duration;
    f->m_size_a = size_a;
    f->m_size_b = size_b;
    f->m_size_c = size_c;
    f->m_log_interval = log_interval;
    f->m_parallel = parallel;
		f->m_copy_matrix = copy_matrix;
    /* Append to list. */
    if (!actions) {
        actions = f;
    } else {
        gst_action *tail = actions;
        while (tail->next) {
            tail = tail->next;
        }
        tail->next = f;
    }
}

void destroy_actions(gst_action *&actions){
	if(actions){
		for (gst_action *f = actions; f; f = f->next) {
			delete f;
		}
	}
}
