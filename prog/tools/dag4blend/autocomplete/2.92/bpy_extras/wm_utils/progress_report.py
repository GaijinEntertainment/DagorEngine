import sys
import typing


class ProgressReport:
    curr_step = None
    ''' '''

    running = None
    ''' '''

    start_time = None
    ''' '''

    steps = None
    ''' '''

    wm = None
    ''' '''

    def enter_substeps(self, nbr, msg):
        ''' 

        '''
        pass

    def finalize(self):
        ''' 

        '''
        pass

    def initialize(self, wm):
        ''' 

        '''
        pass

    def leave_substeps(self, msg):
        ''' 

        '''
        pass

    def start(self):
        ''' 

        '''
        pass

    def step(self, msg, nbr):
        ''' 

        '''
        pass

    def update(self, msg):
        ''' 

        '''
        pass


class ProgressReportSubstep:
    final_msg = None
    ''' '''

    level = None
    ''' '''

    msg = None
    ''' '''

    nbr = None
    ''' '''

    progress = None
    ''' '''

    def enter_substeps(self, nbr, msg):
        ''' 

        '''
        pass

    def leave_substeps(self, msg):
        ''' 

        '''
        pass

    def step(self, msg, nbr):
        ''' 

        '''
        pass
