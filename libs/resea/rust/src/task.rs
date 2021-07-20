use crate::capi::task_t;

pub struct Task {
    id: task_t,
}

impl Task {
    pub unsafe fn from_id(id: task_t) -> Task {
        Task { id }
    }

    pub fn id(&self) -> task_t {
        self.id
    }
}
