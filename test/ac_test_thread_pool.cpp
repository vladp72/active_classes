#include "ac_test_thread_pool.h"

#include <stdlib.h>

#include <actp.h>
#include <acrundown.h>
#include <ackernelobject.h>
#include <acfileobject.h>

void test_ft_to_timepoint_conversion() {
    FILETIME ft;

    GetSystemTimeAsFileTime(&ft);

    std::chrono::system_clock::time_point tp{ac::filetime_to_system_clock_time_point(ft)};

    FILETIME ft2{ac::system_clock_time_point_to_filetime(tp)};

    AC_CODDING_ERROR_IF_NOT(ft.dwHighDateTime == ft2.dwHighDateTime &&
                            ft.dwLowDateTime == ft2.dwLowDateTime);
}

void test_default_tp_submit_work() {
    printf("\n---- test_default_tp_submit_work started\n");

    try {
        constexpr int work_items_to_post{10000};
        std::atomic<int> executed_count{0};

        ac::slim_rundown rundown;
        {
            ac::slim_rundown_join scoped_join(&rundown);

            for (int i = 1; i <= work_items_to_post; ++i) {
                //
                // post half of work items at default (normal) priority
                // and half at high priority
                //
                if (0 == i % 2) {
                    ac::tp::submit_work(
                        [i,
                         &executed_count,
                         rundown_guard = std::move(ac::slim_rundown_lock{&rundown})](
                            ac::tp::callback_instance &instance) {
                            executed_count.fetch_add(1);
                        });
                } else {
                    ac::tp::optional_callback_parameters optional_params;
                    optional_params.priority = TP_CALLBACK_PRIORITY_HIGH;

                    ac::tp::submit_work(
                        [i,
                         &executed_count,
                         rundown_guard = std::move(ac::slim_rundown_lock{&rundown})](
                            ac::tp::callback_instance &instance) {
                            executed_count.fetch_add(1);
                        },
                        &optional_params);
                }
            }

            printf("---- test_default_tp_submit_work waiting to complete\n");
        }

        printf("---- test_default_tp_submit_work validating\n");

        AC_CODDING_ERROR_IF_NOT(executed_count == work_items_to_post);
    } catch (std::exception const &ex) {
        printf("---- test_default_tp_submit_work failed %s\n", ex.what());
    }
    printf("---- test_default_tp_submit_work complete\n");
}

void test_tp_submit_work() {
    printf("\n---- test_tp_submit_work started\n");

    try {
        ac::tp::thread_pool tp{8, 16};

        constexpr int work_items_to_post{10000};
        std::atomic<int> executed_count{0};

        ac::slim_rundown rundown;
        {
            ac::slim_rundown_join scoped_join(&rundown);

            for (int i = 1; i <= work_items_to_post; ++i) {
                //
                // post half of work items at default (normal) priority
                // and half at high priority
                //
                if (0 == i % 2) {
                    tp.submit_work([i,
                                    &executed_count,
                                    rundown_guard = std::move(ac::slim_rundown_lock{
                                        &rundown})](ac::tp::callback_instance &instance) {
                        executed_count.fetch_add(1);
                    });
                } else {
                    ac::tp::optional_callback_parameters optional_params;
                    optional_params.priority = TP_CALLBACK_PRIORITY_HIGH;

                    tp.submit_work(
                        [i,
                         &executed_count,
                         rundown_guard = std::move(ac::slim_rundown_lock{&rundown})](
                            ac::tp::callback_instance &instance) {
                            executed_count.fetch_add(1);
                        },
                        &optional_params);
                }
            }

            printf("---- test_tp_submit_work waiting to complete\n");
        }

        printf("---- test_tp_submit_work validating\n");

        AC_CODDING_ERROR_IF_NOT(executed_count == work_items_to_post);
    } catch (std::exception const &ex) {
        printf("---- test_tp_submit_work failed %s\n", ex.what());
    }
    printf("---- test_tp_submit_work complete\n");
}

void test_default_tp_post() {
    printf("\n---- test_default_tp_post started\n");

    try {
        constexpr size_t work_items_to_post{100};
        std::atomic<int> executed_count{0};

        {
            std::vector<ac::tp::work_item_ptr> work_items;
            work_items.reserve(work_items_to_post);
            {
                for (int i = 1; i <= work_items_to_post; ++i) {
                    switch (i % 4) {
                    case 0: {
                        auto work{ac::tp::make_work_item(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            })};

                        work->post();
                        work->post();

                        work_items.emplace_back(std::move(work));

                        break;
                    }
                    case 1: {
                        ac::tp::optional_callback_parameters optional_params;
                        optional_params.priority = TP_CALLBACK_PRIORITY_HIGH;

                        auto work{ac::tp::make_work_item(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            },
                            &optional_params)};

                        work->post();
                        work->post();

                        work_items.emplace_back(std::move(work));

                        break;
                    }
                    case 2: {
                        auto work{ac::tp::post([i, &executed_count](ac::tp::callback_instance &instance) {
                            executed_count.fetch_add(1);
                        })};

                        work->post();

                        work_items.emplace_back(std::move(work));

                        break;
                    }
                    case 3:
                        [[fallthrough]];
                    default: {
                        ac::tp::optional_callback_parameters optional_params;
                        optional_params.priority = TP_CALLBACK_PRIORITY_HIGH;

                        auto work{ac::tp::post(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            },
                            &optional_params)};

                        work->post();

                        work_items.emplace_back(std::move(work));
                        break;
                    }
                    }
                }
                printf("---- test_default_tp_post waiting to complete\n");
            }
        }

        printf("---- test_default_tp_post validating\n");
        //
        // Each constructed work item is posted 2 times
        //
        AC_CODDING_ERROR_IF_NOT(executed_count == work_items_to_post * 2);

    } catch (std::exception const &ex) {
        printf("---- test_default_tp_post failed %s\n", ex.what());
    }
    printf("---- test_default_tp_post complete\n");
}

void test_tp_post() {
    printf("\n---- test_tp_post started\n");

    try {
        constexpr size_t work_items_to_post{100};
        std::atomic<int> executed_count{0};

        ac::tp::thread_pool tp{8, 16};

        {
            std::vector<ac::tp::work_item_ptr> work_items;
            work_items.reserve(work_items_to_post);
            {
                for (int i = 1; i <= work_items_to_post; ++i) {
                    switch (i % 4) {
                    case 0: {
                        auto work{tp.make_work_item(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            })};

                        work->post();
                        work->post();

                        work_items.emplace_back(std::move(work));

                        break;
                    }
                    case 1: {
                        ac::tp::optional_callback_parameters optional_params;
                        optional_params.priority = TP_CALLBACK_PRIORITY_HIGH;

                        auto work{tp.make_work_item(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            },
                            &optional_params)};

                        work->post();
                        work->post();

                        work_items.emplace_back(std::move(work));

                        break;
                    }
                    case 2: {
                        auto work{tp.post([i, &executed_count](ac::tp::callback_instance &instance) {
                            executed_count.fetch_add(1);
                        })};

                        work->post();

                        work_items.emplace_back(std::move(work));

                        break;
                    }
                    case 3:
                        [[fallthrough]];
                    default: {
                        ac::tp::optional_callback_parameters optional_params;
                        optional_params.priority = TP_CALLBACK_PRIORITY_HIGH;

                        auto work{tp.post(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            },
                            &optional_params)};

                        work->post();

                        work_items.emplace_back(std::move(work));
                        break;
                    }
                    }
                }
                printf("---- test_tp_post waiting to complete\n");
            }
        }

        printf("---- test_tp_post validating\n");
        //
        // Each constructed work item is posted 2 times
        //
        AC_CODDING_ERROR_IF_NOT(executed_count == work_items_to_post * 2);

    } catch (std::exception const &ex) {
        printf("---- test_tp_post failed %s\n", ex.what());
    }
    printf("---- test_tp_post complete\n");
}

void test_default_tp_timer_work_item() {
    printf("\n---- test_default_tp_schedule started\n");

    try {
        constexpr size_t timers_to_schedule{100};
        std::atomic<int> executed_count{0};

        {
            std::vector<ac::tp::timer_work_item_ptr> timers;
            timers.reserve(2 * timers_to_schedule);
            {
                for (int i = 1; i <= timers_to_schedule; ++i) {
                    switch (i % 4) {
                    case 0: {
                        //
                        // schedule in 1 second
                        //
                        auto timer{ac::tp::make_timer_work_item(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            })};

                        AC_CODDING_ERROR_IF(timer->is_set());
                        timer->schedule(ac::tp::seconds{1},
                                        ac::tp::miliseconds{500},
                                        ac::tp::miliseconds{15});
                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));
                        //
                        // schedule at now + 1 second
                        //
                        timer = ac::tp::make_timer_work_item(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            });

                        AC_CODDING_ERROR_IF(timer->is_set());
                        timer->schedule(ac::tp::clock::now() + ac::tp::seconds{1},
                                        ac::tp::miliseconds{500},
                                        ac::tp::miliseconds{15});
                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));

                        break;
                    }
                    case 1: {
                        //
                        // schedule in 1 second
                        //
                        ac::tp::optional_callback_parameters optional_params;
                        optional_params.priority = TP_CALLBACK_PRIORITY_HIGH;

                        auto timer{ac::tp::make_timer_work_item(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            },
                            &optional_params)};

                        AC_CODDING_ERROR_IF(timer->is_set());
                        timer->schedule(ac::tp::seconds{1},
                                        ac::tp::miliseconds{500},
                                        ac::tp::miliseconds{15});
                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));
                        //
                        // schedule at now + 1 second
                        //
                        timer = ac::tp::make_timer_work_item(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            });

                        AC_CODDING_ERROR_IF(timer->is_set());
                        timer->schedule(ac::tp::clock::now() + ac::tp::seconds{1},
                                        ac::tp::miliseconds{500},
                                        ac::tp::miliseconds{15});
                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));

                        break;
                    }
                    case 2: {
                        //
                        // schedule in 1 second
                        //
                        auto timer{ac::tp::schedule(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            },
                            ac::tp::seconds{1},
                            ac::tp::miliseconds{500},
                            ac::tp::miliseconds{15})};

                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));
                        //
                        // schedule at now + 1 second
                        //
                        timer = ac::tp::schedule(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            },
                            ac::tp::clock::now() + ac::tp::seconds{1},
                            ac::tp::miliseconds{500},
                            ac::tp::miliseconds{15});

                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));

                        break;
                    }
                    case 3:
                        [[fallthrough]];
                    default: {
                        //
                        // schedule in 1 second
                        //
                        ac::tp::optional_callback_parameters optional_params;
                        optional_params.priority = TP_CALLBACK_PRIORITY_HIGH;

                        auto timer{ac::tp::schedule(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            },
                            ac::tp::seconds{1},
                            ac::tp::miliseconds{500},
                            ac::tp::miliseconds{15},
                            &optional_params)};

                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));
                        //
                        // schedule at now + 1 second
                        //
                        timer = ac::tp::schedule(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            },
                            ac::tp::clock::now() + ac::tp::seconds{1},
                            ac::tp::miliseconds{500},
                            ac::tp::miliseconds{15});

                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));

                        break;
                    }
                    }
                }
                constexpr DWORD time_to_sleep_in_milliseconds{10 * 1000};
                printf("---- test_default_tp_schedule sleeping for %u "
                       "milliseconds\n",
                       time_to_sleep_in_milliseconds);
                Sleep(time_to_sleep_in_milliseconds);
                printf("---- test_default_tp_schedule waiting to complete\n");
            }
        }

        printf("---- test_default_tp_schedule validating\n");
        //
        // Each constructed work item is posted 2 times
        //
        AC_CODDING_ERROR_IF_NOT(executed_count > timers_to_schedule * 2);

    } catch (std::exception const &ex) {
        printf("---- test_default_tp_schedule failed %s\n", ex.what());
    }
    printf("---- test_default_tp_schedule complete\n");
}

void test_tp_timer_work_item() {
    printf("\n---- test_tp_timer_work_item started\n");

    try {
        constexpr size_t timers_to_schedule{100};
        std::atomic<int> executed_count{0};

        ac::tp::thread_pool tp{8, 16};

        {
            std::vector<ac::tp::timer_work_item_ptr> timers;
            timers.reserve(2 * timers_to_schedule);
            {
                for (int i = 1; i <= timers_to_schedule; ++i) {
                    switch (i % 4) {
                    case 0: {
                        //
                        // schedule in 1 second
                        //
                        auto timer{tp.make_timer_work_item(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            })};

                        AC_CODDING_ERROR_IF(timer->is_set());
                        timer->schedule(ac::tp::seconds{1},
                                        ac::tp::miliseconds{500},
                                        ac::tp::miliseconds{15});
                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));
                        //
                        // schedule at now + 1 second
                        //
                        timer = tp.make_timer_work_item(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            });

                        AC_CODDING_ERROR_IF(timer->is_set());
                        timer->schedule(ac::tp::clock::now() + ac::tp::seconds{1},
                                        ac::tp::miliseconds{500},
                                        ac::tp::miliseconds{15});
                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));

                        break;
                    }
                    case 1: {
                        //
                        // schedule in 1 second
                        //
                        ac::tp::optional_callback_parameters optional_params;
                        optional_params.priority = TP_CALLBACK_PRIORITY_HIGH;

                        auto timer{tp.make_timer_work_item(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            },
                            &optional_params)};

                        AC_CODDING_ERROR_IF(timer->is_set());
                        timer->schedule(ac::tp::seconds{1},
                                        ac::tp::miliseconds{500},
                                        ac::tp::miliseconds{15});
                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));
                        //
                        // schedule at now + 1 second
                        //
                        timer = tp.make_timer_work_item(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            },
                            &optional_params);

                        AC_CODDING_ERROR_IF(timer->is_set());
                        timer->schedule(ac::tp::clock::now() + ac::tp::seconds{1},
                                        ac::tp::miliseconds{500},
                                        ac::tp::miliseconds{15});
                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));

                        break;
                    }
                    case 2: {
                        //
                        // schedule in 1 second
                        //
                        auto timer{tp.schedule(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            },
                            ac::tp::seconds{1},
                            ac::tp::miliseconds{500},
                            ac::tp::miliseconds{15})};

                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));
                        //
                        // schedule at now + 1 second
                        //
                        timer = tp.schedule(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            },
                            ac::tp::clock::now() + ac::tp::seconds{1},
                            ac::tp::miliseconds{500},
                            ac::tp::miliseconds{15});

                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));

                        break;
                    }
                    case 3:
                        [[fallthrough]];
                    default: {
                        //
                        // schedule in 1 second
                        //
                        ac::tp::optional_callback_parameters optional_params;
                        optional_params.priority = TP_CALLBACK_PRIORITY_HIGH;

                        auto timer{tp.schedule(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            },
                            ac::tp::seconds{1},
                            ac::tp::miliseconds{500},
                            ac::tp::miliseconds{15},
                            &optional_params)};

                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));
                        //
                        // schedule at now + 1 second
                        //
                        timer = tp.schedule(
                            [i, &executed_count](ac::tp::callback_instance &instance) {
                                executed_count.fetch_add(1);
                            },
                            ac::tp::clock::now() + ac::tp::seconds{1},
                            ac::tp::miliseconds{500},
                            ac::tp::miliseconds{15},
                            &optional_params);

                        AC_CODDING_ERROR_IF_NOT(timer->is_set());
                        timers.emplace_back(std::move(timer));

                        break;
                    }
                    }
                }
                constexpr DWORD time_to_sleep_in_milliseconds{10 * 1000};
                printf("---- test_tp_timer_work_item sleeping for %u "
                       "milliseconds\n",
                       time_to_sleep_in_milliseconds);
                Sleep(time_to_sleep_in_milliseconds);
                printf("---- test_tp_timer_work_item waiting to complete\n");
            }
        }

        printf("---- test_tp_timer_work_item validating\n");
        //
        // Each constructed work item is posted 2 times
        //
        AC_CODDING_ERROR_IF_NOT(executed_count > timers_to_schedule * 2);

    } catch (std::exception const &ex) {
        printf("---- test_tp_timer_work_item failed %s\n", ex.what());
    }
    printf("---- test_tp_timer_work_item complete\n");
}

void test_default_tp_wait_work_item() {
    printf("\n---- test_default_tp_wait_work_item started\n");

    try {
        constexpr size_t waits_to_schedule{100};
        std::atomic<int> executed_count{0};
        std::atomic<int> timeout_count{0};
        std::atomic<int> signaled_count{0};
        ac::event event{ac::event::automatic, ac::event::signaled};

        {
            std::vector<ac::tp::wait_work_item_ptr> waits;
            waits.reserve(2 * waits_to_schedule);
            {
                for (int i = 1; i <= waits_to_schedule; ++i) {
                    switch (i % 4) {
                    case 0: {
                        //
                        // schedule in 1 second
                        //
                        auto wait{ac::tp::make_wait_work_item(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            })};

                        wait->schedule_wait(event.get_handle(), ac::tp::seconds{1});

                        waits.emplace_back(std::move(wait));
                        //
                        // schedule at now + 1 second
                        //
                        wait = ac::tp::make_wait_work_item(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            });

                        wait->schedule_wait(event.get_handle(),
                                            ac::tp::clock::now() + ac::tp::seconds{1});
                        waits.emplace_back(std::move(wait));

                        break;
                    }
                    case 1: {
                        //
                        // schedule in 1 second
                        //
                        ac::tp::optional_callback_parameters optional_params;
                        optional_params.priority = TP_CALLBACK_PRIORITY_HIGH;

                        auto wait{ac::tp::make_wait_work_item(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            },
                            &optional_params)};

                        wait->schedule_wait(event.get_handle(), ac::tp::seconds{1});
                        waits.emplace_back(std::move(wait));
                        //
                        // schedule at now + 1 second
                        //
                        wait = ac::tp::make_wait_work_item(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            },
                            &optional_params);

                        wait->schedule_wait(event.get_handle(),
                                            ac::tp::clock::now() + ac::tp::seconds{1});
                        waits.emplace_back(std::move(wait));

                        break;
                    }
                    case 2: {
                        //
                        // schedule in 1 second
                        //
                        auto wait{ac::tp::schedule_wait(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            },
                            event.get_handle(),
                            ac::tp::seconds{1})};

                        waits.emplace_back(std::move(wait));
                        //
                        // schedule at now + 1 second
                        //
                        wait = ac::tp::schedule_wait(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            },
                            event.get_handle(),
                            ac::tp::clock::now() + ac::tp::seconds{1});

                        waits.emplace_back(std::move(wait));

                        break;
                    }
                    case 3:
                        [[fallthrough]];
                    default: {
                        //
                        // schedule in 1 second
                        //
                        ac::tp::optional_callback_parameters optional_params;
                        optional_params.priority = TP_CALLBACK_PRIORITY_HIGH;

                        auto wait{ac::tp::schedule_wait(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            },
                            event.get_handle(),
                            ac::tp::seconds{1},
                            &optional_params)};

                        waits.emplace_back(std::move(wait));
                        //
                        // schedule at now + 1 second
                        //
                        wait = ac::tp::schedule_wait(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            },
                            event.get_handle(),
                            ac::tp::clock::now() + ac::tp::seconds{1},
                            &optional_params);

                        waits.emplace_back(std::move(wait));

                        break;
                    }
                    }
                }
                printf("---- test_default_tp_wait_work_item signaling event\n");
                event.set();
                constexpr DWORD time_to_sleep_in_milliseconds{10 * 1000};
                printf("---- test_default_tp_wait_work_item sleeping for %u "
                       "milliseconds\n",
                       time_to_sleep_in_milliseconds);
                Sleep(time_to_sleep_in_milliseconds);
                printf("---- test_default_tp_wait_work_item waiting to "
                       "complete\n");
            }
            printf("---- test_default_tp_wait_work_item  timeout count %i "
                   "signaled count %i\n",
                   timeout_count.load(),
                   signaled_count.load());
        }
        printf("---- test_default_tp_wait_work_item validating\n");

        printf("---- test_default_tp_wait_work_item validating\n");
        //
        // Each constructed work item is posted 2 times
        //
        AC_CODDING_ERROR_IF_NOT(executed_count == waits_to_schedule * 2);

    } catch (std::exception const &ex) {
        printf("---- test_default_tp_wait_work_item failed %s\n", ex.what());
    }
    printf("---- test_default_tp_wait_work_item complete\n");
}

void test_tp_wait_work_item() {
    printf("\n---- test_tp_wait_work_item started\n");

    try {
        constexpr size_t waits_to_schedule{100};
        std::atomic<int> executed_count{0};
        std::atomic<int> timeout_count{0};
        std::atomic<int> signaled_count{0};
        ac::event event{ac::event::automatic, ac::event::signaled};

        ac::tp::thread_pool tp{8, 16};

        {
            std::vector<ac::tp::wait_work_item_ptr> waits;
            waits.reserve(2 * waits_to_schedule);
            {
                for (int i = 1; i <= waits_to_schedule; ++i) {
                    switch (i % 4) {
                    case 0: {
                        //
                        // schedule in 1 second
                        //
                        auto wait{tp.make_wait_work_item(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            })};

                        wait->schedule_wait(event.get_handle(), ac::tp::seconds{1});

                        waits.emplace_back(std::move(wait));
                        //
                        // schedule at now + 1 second
                        //
                        wait = tp.make_wait_work_item(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            });

                        wait->schedule_wait(event.get_handle(),
                                            ac::tp::clock::now() + ac::tp::seconds{1});
                        waits.emplace_back(std::move(wait));

                        break;
                    }
                    case 1: {
                        //
                        // schedule in 1 second
                        //
                        ac::tp::optional_callback_parameters optional_params;
                        optional_params.priority = TP_CALLBACK_PRIORITY_HIGH;

                        auto wait{tp.make_wait_work_item(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            },
                            &optional_params)};

                        wait->schedule_wait(event.get_handle(), ac::tp::seconds{1});
                        waits.emplace_back(std::move(wait));
                        //
                        // schedule at now + 1 second
                        //
                        wait = tp.make_wait_work_item(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            },
                            &optional_params);

                        wait->schedule_wait(event.get_handle(),
                                            ac::tp::clock::now() + ac::tp::seconds{1});
                        waits.emplace_back(std::move(wait));

                        break;
                    }
                    case 2: {
                        //
                        // schedule in 1 second
                        //
                        auto wait{tp.schedule_wait(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            },
                            event.get_handle(),
                            ac::tp::seconds{1})};

                        waits.emplace_back(std::move(wait));
                        //
                        // schedule at now + 1 second
                        //
                        wait = tp.schedule_wait(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            },
                            event.get_handle(),
                            ac::tp::clock::now() + ac::tp::seconds{1});

                        waits.emplace_back(std::move(wait));

                        break;
                    }
                    case 3:
                        [[fallthrough]];
                    default: {
                        //
                        // schedule in 1 second
                        //
                        ac::tp::optional_callback_parameters optional_params;
                        optional_params.priority = TP_CALLBACK_PRIORITY_HIGH;

                        auto wait{tp.schedule_wait(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            },
                            event.get_handle(),
                            ac::tp::seconds{1},
                            &optional_params)};

                        waits.emplace_back(std::move(wait));
                        //
                        // schedule at now + 1 second
                        //
                        wait = tp.schedule_wait(
                            [i, &executed_count, &timeout_count, &signaled_count](
                                ac::tp::callback_instance &instance, TP_WAIT_RESULT wait_result) {
                                executed_count.fetch_add(1);
                                if (WAIT_TIMEOUT == wait_result) {
                                    timeout_count.fetch_add(1);
                                } else {
                                    signaled_count.fetch_add(1);
                                }
                            },
                            event.get_handle(),
                            ac::tp::clock::now() + ac::tp::seconds{1},
                            &optional_params);

                        waits.emplace_back(std::move(wait));

                        break;
                    }
                    }
                }
                printf("---- test_tp_wait_work_item signaling event\n");
                event.set();
                constexpr DWORD time_to_sleep_in_milliseconds{10 * 1000};
                printf("---- test_tp_wait_work_item sleeping for %u "
                       "milliseconds\n",
                       time_to_sleep_in_milliseconds);
                Sleep(time_to_sleep_in_milliseconds);
                printf("---- test_tp_wait_work_item waiting to "
                       "complete\n");
            }
            printf("---- test_tp_wait_work_item  timeout count %i "
                   "signaled count %i\n",
                   timeout_count.load(),
                   signaled_count.load());
        }
        printf("---- test_tp_wait_work_item validating\n");

        printf("---- test_tp_wait_work_item validating\n");
        //
        // Each constructed work item is posted 2 times
        //
        AC_CODDING_ERROR_IF_NOT(executed_count == waits_to_schedule * 2);

    } catch (std::exception const &ex) {
        printf("---- test_tp_wait_work_item failed %s\n", ex.what());
    }
    printf("---- test_tp_wait_work_item complete\n");
}

#define TEST_DEFAULT_TP_IO_HANDLER_FILE_NAME L"foo.tst"
#define TEST_DEFAULT_TP_IO_HANDLER_FILE_SIZE (10LL * 1024LL * 1024LL)
#define TEST_DEFAULT_TP_IO_HANDLER_FILE_IO_SIZE (4096LL * 1024LL)

void test_default_tp_io_handler() {
    printf("\n---- test_default_tp_io_handler started\n");

    std::atomic<bool> io_failure{false};
    constexpr int io_to_start{100};
    std::atomic<int> total_started_count{0};
    std::atomic<int> total_completed_count{0};
    std::atomic<int> total_succeeded_count{0};
    std::atomic<int> total_failed_count{0};
    std::atomic<long long> total_bytes_transfered{0};

    try {
        ac::scoped_file_delete scoped_delete{TEST_DEFAULT_TP_IO_HANDLER_FILE_NAME};

        printf("---- test_default_tp_io_handler opening file\n");

        ac::file_object fo;
        fo.create(TEST_DEFAULT_TP_IO_HANDLER_FILE_NAME,
                  GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  OPEN_ALWAYS,
                  FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED);
        {
            printf("---- test_default_tp_io_handler constructing IO handler\n");

            ac::tp::io_handler_ptr io_handler{ac::tp::make_io_handler(
                fo.get_handle(),
                [&total_completed_count, &total_succeeded_count, &total_failed_count, &total_bytes_transfered, &io_failure](
                    ac::tp::callback_instance &, OVERLAPPED *overlapped, ULONG error, ULONG_PTR bytes_transferred) {
                    std::unique_ptr<OVERLAPPED> overlapped_ptr{overlapped};

                    ++total_completed_count;
                    total_bytes_transfered += bytes_transferred;

                    if (error == ERROR_SUCCESS) {
                        total_succeeded_count += 1;
                    } else {
                        total_failed_count += 1;
                        io_failure = true;
                        printf("\n---- !!! test_default_tp_io_handler IO "
                               "failed !!!"
                               "error. "
                               "0x%x, bytes transferred %u, overlapped 0x%p\n",
                               error,
                               static_cast<DWORD>(bytes_transferred),
                               overlapped);
                    }
                })};
            {
                ac::cbuffer buffer;
                buffer.resize(TEST_DEFAULT_TP_IO_HANDLER_FILE_IO_SIZE);

                printf("---- test_default_tp_io_handler starting IOs\n");

                for (long long i = 0; i < io_to_start && io_failure != true; ++i) {
                    long long offset = (i * TEST_DEFAULT_TP_IO_HANDLER_FILE_IO_SIZE) %
                                       TEST_DEFAULT_TP_IO_HANDLER_FILE_SIZE;
                    std::unique_ptr<OVERLAPPED> overlapped_ptr{
                        std::make_unique<OVERLAPPED>()};
                    overlapped_ptr->hEvent = nullptr;
                    overlapped_ptr->Offset = ac::get_low_dword(offset);
                    overlapped_ptr->OffsetHigh = ac::get_high_dword(offset);

                    total_started_count += 1;

                    auto io_guard{io_handler->start_io()};
                    AC_CODDING_ERROR_IF_NOT(io_guard.is_armed());

                    //
                    // throws on failure; on success of pending completion will go through io_handler
                    //
                    (void) fo.write(&buffer[0],
                                    static_cast<DWORD>(buffer.size()),
                                    overlapped_ptr.get());
                    //
                    // If we successfully started IO then release ownership of the overlapped pointer.
                    // io_handler on completion will take over ownership and will delete this structure
                    //
                    overlapped_ptr.release();
                    //
                    // we successfully initiated IO so we can disarm IO guard
                    //
                    io_guard.disarm();
                }
            }

            printf("---- test_default_tp_io_handler succeeded %i, failed %i, "
                   "bytes transfered %I64u\n",
                   total_succeeded_count.load(),
                   total_failed_count.load(),
                   total_bytes_transfered.load());
        }
        //
        // now that io handler is joined and destroyed all IOs that we started
        // should be completed and we do not expect any IOs to fail
        //
        AC_CODDING_ERROR_IF_NOT(total_succeeded_count == io_to_start);

    } catch (std::exception const &ex) {
        printf("---- test_default_tp_io_handler failed %s\n", ex.what());
    }
    printf("---- test_default_tp_io_handler complete\n");
}

void test_tp_io_handler() {
    printf("\n---- test_tp_io_handler started\n");

    std::atomic<bool> io_failure{false};
    constexpr int io_to_start{100};
    std::atomic<int> total_started_count{0};
    std::atomic<int> total_completed_count{0};
    std::atomic<int> total_succeeded_count{0};
    std::atomic<int> total_failed_count{0};
    std::atomic<long long> total_bytes_transfered{0};

    try {
        ac::tp::thread_pool tp{16, 8};

        ac::scoped_file_delete scoped_delete{TEST_DEFAULT_TP_IO_HANDLER_FILE_NAME};

        printf("---- test_tp_io_handler opening file\n");

        ac::file_object fo;
        fo.create(TEST_DEFAULT_TP_IO_HANDLER_FILE_NAME,
                  GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  OPEN_ALWAYS,
                  FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED);
        {
            printf("---- test_tp_io_handler constructing IO handler\n");

            ac::tp::io_handler_ptr io_handler{tp.make_io_handler(
                fo.get_handle(),
                [&total_completed_count, &total_succeeded_count, &total_failed_count, &total_bytes_transfered, &io_failure](
                    ac::tp::callback_instance &, OVERLAPPED *overlapped, ULONG error, ULONG_PTR bytes_transferred) {
                    std::unique_ptr<OVERLAPPED> overlapped_ptr{overlapped};

                    ++total_completed_count;
                    total_bytes_transfered += bytes_transferred;

                    if (error == ERROR_SUCCESS) {
                        total_succeeded_count += 1;
                    } else {
                        total_failed_count += 1;
                        io_failure = true;
                        printf("\n---- !!! test_tp_io_handler IO "
                               "failed !!!"
                               "error. "
                               "0x%x, bytes transferred %u, overlapped 0x%p\n",
                               error,
                               static_cast<DWORD>(bytes_transferred),
                               overlapped);
                    }
                })};
            {
                ac::cbuffer buffer;
                buffer.resize(TEST_DEFAULT_TP_IO_HANDLER_FILE_IO_SIZE);

                printf("---- test_tp_io_handler starting IOs\n");

                for (long long i = 0; i < io_to_start && io_failure != true; ++i) {
                    long long offset = (i * TEST_DEFAULT_TP_IO_HANDLER_FILE_IO_SIZE) %
                                       TEST_DEFAULT_TP_IO_HANDLER_FILE_SIZE;
                    std::unique_ptr<OVERLAPPED> overlapped_ptr{
                        std::make_unique<OVERLAPPED>()};
                    overlapped_ptr->hEvent = nullptr;
                    overlapped_ptr->Offset = ac::get_low_dword(offset);
                    overlapped_ptr->OffsetHigh = ac::get_high_dword(offset);

                    total_started_count += 1;

                    auto io_guard{io_handler->start_io()};
                    AC_CODDING_ERROR_IF_NOT(io_guard.is_armed());

                    //
                    // throws on failure; on success of pending completion will go through io_handler
                    //
                    (void) fo.write(&buffer[0],
                                    static_cast<DWORD>(buffer.size()),
                                    overlapped_ptr.get());
                    //
                    // If we successfully started IO then release ownership of the overlapped pointer.
                    // io_handler on completion will take over ownership and will delete this structure
                    //
                    overlapped_ptr.release();
                    //
                    // we successfully initiated IO so we can disarm IO guard
                    //
                    io_guard.disarm();
                }
            }

            printf("---- test_tp_io_handler succeeded %i, failed %i, "
                   "bytes transfered %I64u\n",
                   total_succeeded_count.load(),
                   total_failed_count.load(),
                   total_bytes_transfered.load());
        }
        //
        // now that io handler is joined and destroyed all IOs that we started
        // should be completed and we do not expect any IOs to fail
        //
        AC_CODDING_ERROR_IF_NOT(total_succeeded_count == io_to_start);

    } catch (std::exception const &ex) {
        printf("---- test_tp_io_handler failed %s\n", ex.what());
    }
    printf("---- test_tp_io_handler complete\n");
}
