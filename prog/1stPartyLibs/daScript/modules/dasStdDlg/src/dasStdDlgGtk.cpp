#include "daScript/misc/platform.h"

#include "dasStdDlg.h"

#include <gtk/gtk.h>

namespace das {

    /*
        TODO: support filters
    */

    void StdDlgInit() {
        gtk_init (0, nullptr);
    }

	bool GetOkCancelFromUser(const char * caption, const char * body) {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(nullptr,
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_QUESTION,
                    GTK_BUTTONS_YES_NO,
                    "%s", body);
        gtk_window_set_title(GTK_WINDOW(dialog), caption);
        gint res = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return (res == GTK_RESPONSE_YES);
	}

	bool GetOkFromUser(const char * caption, const char * body) {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(nullptr,
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_INFO,
                    GTK_BUTTONS_OK,
                    "%s", body);
        gtk_window_set_title(GTK_WINDOW(dialog), caption);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return true;
	}

	string GetSaveFileFromUser( const char * initialFileName , const char * initialPath, const char * filter ) {
        string result;
        GtkWidget *dialog;
        GtkFileChooser *chooser;
        GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
        gint res;
        dialog = gtk_file_chooser_dialog_new("Save File",
                                            NULL,
                                            action,
                                            "_Cancel",
                                            GTK_RESPONSE_CANCEL,
                                            "_Save",
                                            GTK_RESPONSE_ACCEPT,
                                            NULL);
        chooser = GTK_FILE_CHOOSER(dialog);
        gtk_file_chooser_set_do_overwrite_confirmation(chooser,TRUE);
        gtk_file_chooser_set_current_name(chooser,initialFileName);
        gtk_file_chooser_set_current_folder(chooser,initialPath);
        res = gtk_dialog_run(GTK_DIALOG(dialog));
        if (res == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(chooser);
            result = filename;
            g_free(filename);
        }
        gtk_widget_destroy (dialog);
        return result;
	}

	string GetOpenFileFromUser ( const char * initialPath, const char * filter ) {
        string result;
        GtkWidget *dialog;
        GtkFileChooser *chooser;
        GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
        gint res;
        dialog = gtk_file_chooser_dialog_new("Open File",
                                            NULL,
                                            action,
                                            "_Cancel",
                                            GTK_RESPONSE_CANCEL,
                                            "_Open",
                                            GTK_RESPONSE_ACCEPT,
                                            NULL);
        chooser = GTK_FILE_CHOOSER(dialog);
        gtk_file_chooser_set_current_folder(chooser,initialPath);
        res = gtk_dialog_run(GTK_DIALOG(dialog));
        if (res == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(chooser);
            result = filename;
            g_free(filename);
        }
        gtk_widget_destroy(dialog);
        return result;
	}
}
